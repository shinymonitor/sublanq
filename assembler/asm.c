#include <stdio.h>
#include <stdlib.h>

#include "picoct.h"

#include <stdint.h>
#include <assert.h>

#define WORD_SIZE 16
#define WORD_UTYPE uint16_t
#define WORD_STYPE int16_t
#define WORD_MAX UINT16_MAX

typedef enum {
    TOKEN_START, TOKEN_ASSIGN, TOKEN_ARRAY, TOKEN_COMMA, TOKEN_ALLOC, TOKEN_HYPHEN, 
    TOKEN__INST_BEGIN,
    TOKEN_INST_ZER, TOKEN_INST_INC, TOKEN_INST_DEC, 
    TOKEN_INST_NEG, TOKEN_INST_ADD, TOKEN_INST_SUB, TOKEN_INST_MUL, TOKEN_INST_DIV, TOKEN_INST_MOD, 
    TOKEN_INST_JMP, TOKEN_INST_JLE, TOKEN_INST_JLZ, TOKEN_INST_JEZ, TOKEN_INST_JGE, TOKEN_INST_JGZ, TOKEN_INST_SJP, TOKEN_INST_LJP, 
    TOKEN_INST_MOV, TOKEN_INST_ADR, TOKEN_INST_DRD, TOKEN_INST_DWT, 
    TOKEN_INST_INP, TOKEN_INST_OUT, TOKEN_INST_HLT,
    TOKEN__INST_END,
    TOKEN_LABEL_DECL, TOKEN_LABEL_USE,
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_EOS, TOKEN_UNKNOWN,
} TokenType;

#define MAX_IDENTIFIER_LENGTH 2048

TokenType token = TOKEN_UNKNOWN;
static char token_string_buffer[MAX_IDENTIFIER_LENGTH + 1] = {0};
static char temp_token_string_buffer[MAX_IDENTIFIER_LENGTH + 2] = {0};
static WORD_UTYPE token_number = 0;

static PICOCT_Match keyword_table[] = {
    {"zer",  TOKEN_INST_ZER}, {"inc",  TOKEN_INST_INC}, {"dec",  TOKEN_INST_DEC},
    {"neg",  TOKEN_INST_NEG}, {"add",  TOKEN_INST_ADD}, {"sub",  TOKEN_INST_SUB}, {"mul",  TOKEN_INST_MUL}, {"div",  TOKEN_INST_DIV}, {"mod",  TOKEN_INST_MOD},
    {"jmp",  TOKEN_INST_JMP}, {"jle",  TOKEN_INST_JLE}, {"jlz",  TOKEN_INST_JLZ}, {"jez",  TOKEN_INST_JEZ}, {"jge",  TOKEN_INST_JGE}, {"jgz",  TOKEN_INST_JGZ}, {"sjp",  TOKEN_INST_SJP}, {"ljp",  TOKEN_INST_LJP},
    {"mov",  TOKEN_INST_MOV}, {"adr",  TOKEN_INST_ADR}, {"drd",  TOKEN_INST_DRD}, {"dwt",  TOKEN_INST_DWT}, 
    {"inp",  TOKEN_INST_INP}, {"out",  TOKEN_INST_OUT}, {"hlt",  TOKEN_INST_HLT},
};
static PICOCT_Match symbol_table[] = {
    {"__start__", TOKEN_START},
    {"=",         TOKEN_ASSIGN},
    {"*",         TOKEN_ARRAY},
    {",",         TOKEN_COMMA},
    {"|",         TOKEN_ALLOC},
    {"$",         TOKEN_LABEL_DECL},
    {"@",         TOKEN_LABEL_USE},
    {"-",         TOKEN_HYPHEN},
};

static PICOCT_Context ctx = {
    .comment_marker           = ";",
    .keyword_match_table       = keyword_table,
    .keyword_match_table_count = sizeof(keyword_table) / sizeof(keyword_table[0]),
    .symbol_match_table        = symbol_table,
    .symbol_match_table_count  = sizeof(symbol_table)  / sizeof(symbol_table[0]),
    .token_string_buffer       = token_string_buffer,
    .token_string_capacity     = MAX_IDENTIFIER_LENGTH,
};
static const char** token_type_names = (const char*[]) {
    "TOKEN_START", "TOKEN_ASSIGN (=)", "TOKEN_ARRAY (*)", "TOKEN_COMMA (,)", "TOKEN_ALLOC (|)", "TOKEN_HYPHEN (-)", 
    "",
    "INST_ZER", "INST_INC", "INST_DEC", 
    "INST_NEG", "INST_ADD", "INST_SUB", "INST_MUL", "INST_DIV", "INST_MOD", 
    "INST_JMP", "INST_JLE", "INST_JLZ", "INST_JEZ", "INST_JGE", "INST_JGZ", "INST_SJP", "INST_LJP", 
    "INST_MOV", "INST_ADR", "INST_DRD", "INST_DWT", 
    "INST_INP", "INST_OUT", "INST_HLT",
    "",
    "TOKEN_LABEL_DECL ($)", "TOKEN_LABEL_USE (@)",
    "IDENTIFIER", "NUMBER", "END_OF_SOURCE", "UNKNOWN",
};

typedef enum {
    IST_NONE,
    IST_ADDR,
    IST_IMMADDR_ADDR,
    IST_LABEL,
    IST_ADDR_LABEL,
    IST_LABELDEREF_ADDR,
    IST_ADDRDEREF_ADDR,
    IST_PORT_ADDR,
    IST_IMMADDR_PORT,
} InstSyntaxType;

static InstSyntaxType inst_syntax_types[256] = {
    [TOKEN_INST_ZER] = IST_ADDR, [TOKEN_INST_INC] = IST_ADDR, [TOKEN_INST_DEC] = IST_ADDR, 
    [TOKEN_INST_NEG] = IST_ADDR, [TOKEN_INST_ADD] = IST_IMMADDR_ADDR, [TOKEN_INST_SUB] = IST_IMMADDR_ADDR, [TOKEN_INST_MUL] = IST_IMMADDR_ADDR, [TOKEN_INST_DIV] = IST_IMMADDR_ADDR, [TOKEN_INST_MOD] = IST_IMMADDR_ADDR, 
    [TOKEN_INST_JMP] = IST_LABEL, [TOKEN_INST_JLE] = IST_ADDR_LABEL, [TOKEN_INST_JLZ] = IST_ADDR_LABEL, [TOKEN_INST_JEZ] = IST_ADDR_LABEL, [TOKEN_INST_JGE] = IST_ADDR_LABEL, [TOKEN_INST_JGZ] = IST_ADDR_LABEL, [TOKEN_INST_SJP] = IST_LABELDEREF_ADDR, [TOKEN_INST_LJP] = IST_ADDR, 
    [TOKEN_INST_MOV] = IST_IMMADDR_ADDR, [TOKEN_INST_ADR] = IST_ADDRDEREF_ADDR, [TOKEN_INST_DRD] = IST_IMMADDR_ADDR, [TOKEN_INST_DWT] = IST_IMMADDR_ADDR, 
    [TOKEN_INST_INP] = IST_PORT_ADDR, [TOKEN_INST_OUT] = IST_IMMADDR_PORT, [TOKEN_INST_HLT] = IST_NONE,
};

typedef enum {
    I = 256, E, A, B, Z, M, O, P, Q, R, S, N
} ABType;

typedef struct {
    uint16_t a, b, c;
} CodeGenType;

static CodeGenType INST_ZER_code_gen[] = {{A, A, I}};
static CodeGenType INST_INC_code_gen[] = {{M, A, I}};
static CodeGenType INST_DEC_code_gen[] = {{O, A, I}};
static CodeGenType INST_NEG_code_gen[] = {{Z, Z, I}, {P, P, I}, {A, P, I}, {P, Z, I}, {A, A, I}, {Z, A, I}};
static CodeGenType INST_ADD_code_gen[] = {{Z, Z, I}, {A, Z, I}, {Z, B, I}};
static CodeGenType INST_SUB_code_gen[] = {{A, B, I}};
static CodeGenType INST_MUL_code_gen[] = {
    {S, S, I}, {Q, Q, I},  // zer s, q
    {Z, Z, I}, {Z, A, 7}, // jle a @neg_q
    {A, Z, I}, {Z, Q, I}, // mov a q
    {Z, Z, 9},  // jmp @no_neg_q
    {A, Q, I}, // $neg_q, mov -a q
    {M, S, I}, // mov 1 s
    {R, R, I}, {B, R, I}, {B, B, I}, // $no_neg_q, mov -b r, zer b
    {Z, Q, 16}, {O, Q, I}, {R, B, I}, {Z, Z, 12}, // $loop, jle q @end, dec q, sub r b, jmp @loop
    {Z, S, E}, // $end, jle s @no_neg_b
    {P, P, I}, {B, P, I}, {P, Z, I}, {B, B, I}, {Z, B, I} // neg b, $no_neg_b
};
static CodeGenType INST_DIV_code_gen[] = {
    {S, S, I}, {Q, Q, I}, {R, R, I}, // zer s, q, r
    {Z, Z, I}, {Z, A, 8}, // jle a @neg_q
    {A, Z, I}, {Z, Q, I}, // mov a q
    {Z, Z, 10},  // jmp @no_neg_q
    {A, Q, I}, // $neg_q, mov -a q
    {M, S, I}, // mov 1 s
    {Z, B, 14}, // $no_neg_q, jle b @neg_r
    {B, Z, I}, {Z, R, I}, // mov b r
    {Z, Z, 19},  // jmp @no_neg_r
    {B, R, I}, // $neg_r, mov -b r
    {Z, S, 18}, {S, S, I}, {Z, Z, 19}, {M, S, I}, // jle s @_neg_r, zer s, jmp $no_neg_r, $_neg_r mov 1 s
    {B, B, I}, // $no_neg_r, zer b
    {Q, R, I}, {P, P, I}, {R, P, I}, {Z, P, 25}, {Z, Z, 27}, {M, B, I}, {Z, Z, 20}, // $loop, sub q r, jlz r @end, inc b, jmp @loop
    {Z, S, E}, // $end, jle s @no_neg_b
    {P, P, I}, {B, P, I}, {P, Z, I}, {B, B, I}, {Z, B, I} // neg b, $no_neg_b
};
static CodeGenType INST_MOD_code_gen[] = {
    {S, S, I}, {Q, Q, I}, {R, R, I}, // zer s, q, r
    {Z, Z, I}, {Z, A, 8}, // jle a @neg_q
    {A, Z, I}, {Z, Q, I}, // mov a q
    {Z, Z, 9},  // jmp @no_neg_q
    {A, Q, I}, // $neg_q, mov -a q
    {Z, B, 13}, // $no_neg_q, jle b @neg_r
    {B, Z, I}, {Z, R, I}, // mov b r
    {Z, Z, 15},  // jmp @loop
    {B, R, I}, // $neg_r, mov -b r
    {M, S, I}, // mov 1 s
    {Q, R, I}, {P, P, I}, {R, P, I}, {Z, P, 15}, {Q, Z, I}, {Z, R, I}, // $loop, sub q r, jge r @loop, add q r
    {B, B, I}, // zer b 
    {Z, Z, I}, {Z, S, 26}, // jle s @neg_b
    {R, B, I}, // mov -r b
    {Z, Z, E},  // jmp @e
    {R, Z, I}, {Z, B, I}, // $neg_b, mov r b
};
static CodeGenType INST_JMP_code_gen[] = {{Z, Z, A}};
static CodeGenType INST_JLE_code_gen[] = {{Z, Z, I}, {Z, A, B}};
static CodeGenType INST_JLZ_code_gen[] = {{Z, Z, I}, {P, P, I}, {A, P, I}, {Z, P, E}, {Z, Z, B}};
static CodeGenType INST_JEZ_code_gen[] = {{Z, Z, I}, {P, P, I}, {Z, A, 4}, {Z, Z, E}, {A, P, I}, {Z, P, B}};
static CodeGenType INST_JGE_code_gen[] = {{Z, Z, I}, {P, P, I}, {A, P, I}, {Z, P, B}};
static CodeGenType INST_JGZ_code_gen[] = {{Z, Z, I}, {Z, A, E}, {Z, Z, B}};
static CodeGenType INST_SJP_code_gen[] = {{Z, Z, I}, {A, Z, I}, {B, B, I}, {Z, B, I}};
static CodeGenType INST_LJP_code_gen[] = {{Z, Z, I}, {14, 14, I}, {A, Z, I}, {Z, 14, I}, {Z, Z, 0}};
static CodeGenType INST_MOV_code_gen[] = {{Z, Z, I}, {A, Z, I}, {B, B, I}, {Z, B, I}};
static CodeGenType INST_ADR_code_gen[] = {{Z, Z, I}, {A, Z, I}, {B, B, I}, {Z, B, I}};
static CodeGenType INST_DRD_code_gen[] = {{Z, Z, I}, {15, 15, I}, {A, Z, I}, {Z, 15, I}, {Z, Z, I}, {0, Z, I}, {B, B, I}, {Z, B, I}};
static CodeGenType INST_DWT_code_gen[] = {{Z, Z, I}, {27, 27, I}, {28, 28, I}, {34, 34, I}, {B, Z, I}, {Z, 27, I}, {Z, 28, I}, {Z, 34, I}, {Z, Z, I}, {0, 0, I}, {A, Z, I}, {Z, 0, I}};
static CodeGenType INST_INP_code_gen[] = {{N, B, A}};
static CodeGenType INST_OUT_code_gen[] = {{A, N, B}};
static CodeGenType INST_HLT_code_gen[] = {{Z, Z, N}};

static size_t inst_code_gen_sizes[256] = {
    [TOKEN_INST_ZER] = (sizeof(INST_ZER_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_INC] = (sizeof(INST_INC_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_DEC] = (sizeof(INST_DEC_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_NEG] = (sizeof(INST_NEG_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_ADD] = (sizeof(INST_ADD_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_SUB] = (sizeof(INST_SUB_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_MUL] = (sizeof(INST_MUL_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_DIV] = (sizeof(INST_DIV_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_MOD] = (sizeof(INST_MOD_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JMP] = (sizeof(INST_JMP_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JLE] = (sizeof(INST_JLE_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JLZ] = (sizeof(INST_JLZ_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JEZ] = (sizeof(INST_JEZ_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JGE] = (sizeof(INST_JGE_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_JGZ] = (sizeof(INST_JGZ_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_SJP] = (sizeof(INST_SJP_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_LJP] = (sizeof(INST_LJP_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_MOV] = (sizeof(INST_MOV_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_ADR] = (sizeof(INST_ADR_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_DRD] = (sizeof(INST_DRD_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_DWT] = (sizeof(INST_DWT_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_INP] = (sizeof(INST_INP_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_OUT] = (sizeof(INST_OUT_code_gen)/sizeof(CodeGenType)), 
    [TOKEN_INST_HLT] = (sizeof(INST_HLT_code_gen)/sizeof(CodeGenType)),
};

static CodeGenType* inst_code_gen[256] = {
    [TOKEN_INST_ZER] = INST_ZER_code_gen,
    [TOKEN_INST_INC] = INST_INC_code_gen,
    [TOKEN_INST_DEC] = INST_DEC_code_gen,
    [TOKEN_INST_NEG] = INST_NEG_code_gen,
    [TOKEN_INST_ADD] = INST_ADD_code_gen,
    [TOKEN_INST_SUB] = INST_SUB_code_gen,
    [TOKEN_INST_MUL] = INST_MUL_code_gen,
    [TOKEN_INST_DIV] = INST_DIV_code_gen,
    [TOKEN_INST_MOD] = INST_MOD_code_gen,
    [TOKEN_INST_JMP] = INST_JMP_code_gen,
    [TOKEN_INST_JLE] = INST_JLE_code_gen,
    [TOKEN_INST_JLZ] = INST_JLZ_code_gen,
    [TOKEN_INST_JEZ] = INST_JEZ_code_gen,
    [TOKEN_INST_JGE] = INST_JGE_code_gen,
    [TOKEN_INST_JGZ] = INST_JGZ_code_gen,
    [TOKEN_INST_SJP] = INST_SJP_code_gen,
    [TOKEN_INST_LJP] = INST_LJP_code_gen,
    [TOKEN_INST_MOV] = INST_MOV_code_gen,
    [TOKEN_INST_ADR] = INST_ADR_code_gen,
    [TOKEN_INST_DRD] = INST_DRD_code_gen,
    [TOKEN_INST_DWT] = INST_DWT_code_gen,
    [TOKEN_INST_INP] = INST_INP_code_gen,
    [TOKEN_INST_OUT] = INST_OUT_code_gen,
    [TOKEN_INST_HLT] = INST_HLT_code_gen,
};

static inline void tokenize(void) {
    PICOCT_tokenize(&ctx);
    if (ctx.token_type == PICOCT_EOS) token = TOKEN_EOS;
    else if (ctx.token_type == PICOCT_IDENTIFIER) token = TOKEN_IDENTIFIER;
    else if (ctx.token_type == PICOCT_NUMBER) {
        token = TOKEN_NUMBER;
        token_number = (WORD_UTYPE)(WORD_STYPE)(ctx.token_number);
    }
    else if (ctx.token_type == PICOCT_MATCH && ctx.match_type == TOKEN_HYPHEN) { 
        PICOCT_tokenize(&ctx);
        if (ctx.token_type != PICOCT_NUMBER) { token = TOKEN_UNKNOWN; return; }
        token = TOKEN_NUMBER;
        token_number = (WORD_UTYPE)(WORD_STYPE)(-ctx.token_number);
    }
    else if (ctx.token_type == PICOCT_MATCH && ctx.match_type == TOKEN_LABEL_DECL) { 
        PICOCT_tokenize(&ctx);
        if (ctx.token_type != PICOCT_IDENTIFIER) { token = TOKEN_UNKNOWN; return; }
        token = TOKEN_LABEL_DECL;
    }
    else if (ctx.token_type == PICOCT_MATCH && ctx.match_type == TOKEN_LABEL_USE) { 
        PICOCT_tokenize(&ctx);
        if (ctx.token_type != PICOCT_IDENTIFIER) { token = TOKEN_UNKNOWN; return; }
        token = TOKEN_LABEL_USE;
    }
    else if (ctx.token_type == PICOCT_MATCH) token = ctx.match_type;
    else token = TOKEN_UNKNOWN;
}

// hash table interface - assosiative array for now
#define HTI_SIZE 256
char hti_ar_str[HTI_SIZE][MAX_IDENTIFIER_LENGTH] = {0};
WORD_UTYPE hti_ar_val[HTI_SIZE] = {0};
static inline void hti_set(const char* identifier, WORD_UTYPE value) {
    for (size_t i = 0; i < HTI_SIZE; i++) {
        if (hti_ar_str[i][0] == '\0') {
            strcpy(hti_ar_str[i], identifier);
            hti_ar_val[i] = value;
            return;
        }
        else if (strcmp(hti_ar_str[i], identifier) == 0) {
            hti_ar_val[i] = value;
            return;
        }
    }
    PICOCT_error_printf(&ctx, "Symbol table full");
}
static inline void hti_get(const char* identifier, WORD_UTYPE* value) {
    for (size_t i = 0; i < HTI_SIZE; i++) {
        if (strcmp(hti_ar_str[i], identifier) == 0) {
            *value = hti_ar_val[i];
            return;
        }
    }
}
static inline bool hti_exist(const char* identifier) {
    for (size_t i = 0; i < HTI_SIZE; i++) {
        if (strcmp(hti_ar_str[i], identifier) == 0) return true;
    }
    return false;
}


WORD_UTYPE *binary = NULL;
size_t binary_capacity = 0;
size_t binary_idx = 0;

static inline void binary_reserve(void) {
    if (binary_idx < binary_capacity) return;
    if (binary_idx >= WORD_MAX) { fprintf(stderr, "Binary size exceeded max\n"); exit(1); }
    binary_capacity = binary_capacity ? binary_capacity * 2 : 4096;
    binary = realloc(binary, binary_capacity * sizeof(WORD_UTYPE));
    if (!binary) { fprintf(stderr, "Binary memory alloc failed\n"); exit(1); }
}
static inline void binary_push(WORD_UTYPE val) {
    binary_reserve();
    binary[binary_idx++] = val;
}

static inline void add_binary_header(void){
    #define Z_ADDR 3
    #define M_ADDR 4
    #define O_ADDR 5
    #define P_ADDR 6
    #define Q_ADDR 7
    #define R_ADDR 8
    #define S_ADDR 9
    #define N_ADDR ((WORD_UTYPE)-1)
    binary_push(0);
    binary_push(0);
    binary_push(10);
    binary_push(0);
    binary_push(-1);
    binary_push(1);
    binary_push(0);
    binary_push(0);
    binary_push(0);
    binary_push(0);
}
static inline void add_value(WORD_UTYPE value) {
    binary_push(value);
    ++binary[2];
}
static inline void add_variable(const char* identifier, WORD_UTYPE value) {
    hti_set(identifier, binary_idx);
    add_value(value);
}
static inline void add_inst(WORD_UTYPE a, WORD_UTYPE b, WORD_UTYPE c) {
    binary_push(a);
    binary_push(b);
    binary_push(c);
}

static inline void print_state(void) {
    printf("{ ");
    for (size_t i = 0; i < HTI_SIZE; ++i) if (hti_ar_str[i][0] != '\0') printf("%s : %d, ", hti_ar_str[i], hti_ar_val[i]);
    printf("}\n");
    for (size_t i = 0; i < binary_idx; ++i) printf("%d ", binary[i]);
    printf("\n");
}
static inline void print_inst_costs(void) {
    printf("{ ");
    for (size_t i = TOKEN__INST_BEGIN + 1; i < TOKEN__INST_END; ++i) printf("%s = %zu, ", token_type_names[i], inst_code_gen_sizes[i]);
    printf("}\n");
}

static inline void expect_token(TokenType type){
    if (token != type) PICOCT_error_printf(&ctx, "Expected token type %s but got %s", token_type_names[type], token_type_names[token]);
}
static inline void valid_token(){
    if (token == TOKEN_UNKNOWN || token == TOKEN__INST_BEGIN || token == TOKEN__INST_END) PICOCT_error_printf(&ctx, "Unknown token type encountered");
}
static inline void expect_identifier_exist(const char* identifier) {
    if (!hti_exist(identifier)) PICOCT_error_printf(&ctx, "Undeclared identifier \'%s\' encountered", identifier);
}
static inline void expect_identifier_addr_exist(const char* identifier) {
    if (!hti_exist(identifier)) PICOCT_error_printf(&ctx, "Undeclared identifier address \'%s\' encountered", identifier);
}
static inline void expect_label_exist(const char* label) {
    if (!hti_exist(label)) PICOCT_error_printf(&ctx, "Undeclared label \'%s\' encountered", label);
}
static inline void expect_label_addr_exist(const char* label) {
    if (!hti_exist(label)) PICOCT_error_printf(&ctx, "Undeclared label address \'%s\' encountered", label);
}

bool debug_mode = true;
static WORD_UTYPE temp_hti_get_value = 0;
static bool start_found = false;
PICOCT_Cursor saved_cursor = {0};
size_t code_gen_offset = 0;
TokenType inst = TOKEN_UNKNOWN;
WORD_UTYPE inst_a = 0, inst_b = 0;

static inline bool data_section_pass(void) {
    valid_token();
    if (token == TOKEN_START) {
        if (start_found) PICOCT_error_printf(&ctx, "Redeclaration of __start__ symbol");
        start_found = true;
        return false;
    }
    expect_token(TOKEN_IDENTIFIER);
    if (hti_exist(token_string_buffer)) PICOCT_error_printf(&ctx, "Redeclaration of identifier %s", token_string_buffer);
    if (debug_mode) printf("IDENTIFIER: %s\n", token_string_buffer);
    strncpy(temp_token_string_buffer, token_string_buffer, sizeof(temp_token_string_buffer));
    tokenize();
    if (token == TOKEN_ASSIGN) {
        tokenize();
        expect_token(TOKEN_NUMBER);
        if (debug_mode) printf("ASSIGN NUMBER: %d (%d)\n", token_number, (WORD_STYPE)token_number);
        add_variable(temp_token_string_buffer, token_number);
        tokenize();
    }
    else if (token == TOKEN_ARRAY) {
        add_variable(temp_token_string_buffer, 0);
        hti_get(temp_token_string_buffer, &temp_hti_get_value);
        binary[temp_hti_get_value] = temp_hti_get_value + 1;
        tokenize();
        while (true) {
            expect_token(TOKEN_NUMBER);
            if (debug_mode) printf("ARRAY NUMBER: %d (%d)\n", token_number, (WORD_STYPE)token_number);
            add_value(token_number);
            tokenize();
            if (token != TOKEN_COMMA) break;
            tokenize();
        }
    }
    else if (token == TOKEN_ALLOC) {
        add_variable(temp_token_string_buffer, 0);
        hti_get(temp_token_string_buffer, &temp_hti_get_value);
        binary[temp_hti_get_value] = temp_hti_get_value + 1;
        tokenize();
        expect_token(TOKEN_NUMBER);
        if (debug_mode) printf("ALLOCATION SIZE: %d (%d)\n", token_number, (WORD_STYPE)token_number);
        if ((WORD_STYPE)token_number < 0) PICOCT_error_printf(&ctx, "Cannot allocate a buffer of size of a negative number");
        for (size_t i = 0; i < (size_t)(token_number - 1); ++i) add_value(0);
        tokenize();
    }
    else if (token == TOKEN_COMMA) {
        add_variable(temp_token_string_buffer, 0);
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        if (debug_mode) printf("MULTI DECL IDENTIFIER: %s \n", token_string_buffer);
        add_variable(token_string_buffer, 0);
        tokenize();
        while (true) {
            if (token != TOKEN_COMMA) break;
            tokenize();
            expect_token(TOKEN_IDENTIFIER);
            if (debug_mode) printf("MULTI DECL IDENTIFIER: %s \n", token_string_buffer);
            add_variable(token_string_buffer, 0);
            tokenize();
        }
    }
    else PICOCT_error_printf(&ctx, "Expected assignment (=), array declaration (*), allocation (|) or comma (,) for multi declaration");
    return true;
}

static inline void first_pass(void) {
    valid_token();
    if ((token > TOKEN__INST_BEGIN && token < TOKEN__INST_END) && inst_syntax_types[token] == IST_PORT_ADDR) { 
        tokenize(); 
    }
    else if ((token > TOKEN__INST_BEGIN && token < TOKEN__INST_END) && inst_syntax_types[token] == IST_IMMADDR_PORT) {
        tokenize();
        if (token == TOKEN_NUMBER) {
            snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "#%d", token_number);
            if (debug_mode) printf("CONSTANT: %s %d \n", temp_token_string_buffer, token_number);
            if (!hti_exist(temp_token_string_buffer)) add_variable(temp_token_string_buffer, token_number);
        }
        tokenize();
    }
    else if ((token > TOKEN__INST_BEGIN && token < TOKEN__INST_END) && inst_syntax_types[token] == IST_LABELDEREF_ADDR) {
        tokenize();
        expect_token(TOKEN_LABEL_USE);
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "^%s", token_string_buffer);
        if (debug_mode) printf("LABEL ADDRESS: %s \n", temp_token_string_buffer);
        add_variable(temp_token_string_buffer, 0);
    }
    else if ((token > TOKEN__INST_BEGIN && token < TOKEN__INST_END) && inst_syntax_types[token] == IST_ADDRDEREF_ADDR) {
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        if (!hti_exist(token_string_buffer)) PICOCT_error_printf(&ctx, "Cannot get address of an undeclared identifier");
        hti_get(token_string_buffer, &temp_hti_get_value);
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "&%s", token_string_buffer);
        if (debug_mode) printf("IDENTIFIER ADDRESS: %s %d \n", temp_token_string_buffer, temp_hti_get_value);
        add_variable(temp_token_string_buffer, temp_hti_get_value);
    }
    else if (token == TOKEN_NUMBER) {
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "#%d", token_number);
        if (debug_mode) printf("CONSTANT: %s %d \n", temp_token_string_buffer, token_number);
        if (!hti_exist(temp_token_string_buffer)) add_variable(temp_token_string_buffer, token_number);
    }
    tokenize();
}

static inline void second_pass(void) {
    valid_token();
    if ((token > TOKEN__INST_BEGIN && token < TOKEN__INST_END)) {
        if (debug_mode) printf("INSTRUCTION: %s\n", token_type_names[token]);
        code_gen_offset += inst_code_gen_sizes[token] * 3;
    }
    else if (token == TOKEN_LABEL_DECL) {
        if (debug_mode) printf("LABEL DECLARATION: %s %zu \n", token_string_buffer, binary_idx + code_gen_offset);
        if (hti_exist(token_string_buffer)) PICOCT_error_printf(&ctx, "Redeclaration of label %s", token_string_buffer);
        hti_set(token_string_buffer, binary_idx + code_gen_offset);
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "^%s", token_string_buffer);
        if (hti_exist(temp_token_string_buffer)) {
            hti_get(temp_token_string_buffer, &temp_hti_get_value);
            if (debug_mode) printf("UPDATE LABEL ADDRESS: %s %d %zu \n", temp_token_string_buffer, temp_hti_get_value, binary_idx + code_gen_offset);
            binary[temp_hti_get_value] = binary_idx + code_gen_offset;
        }
    }
    tokenize();
}

static inline void third_pass(void) {
    valid_token();
    if (token == TOKEN_LABEL_DECL) { 
        if (debug_mode) printf("LABEL DECLARATION: %s\n", token_string_buffer);
        tokenize(); 
        return; 
    }
    if (token < TOKEN__INST_BEGIN || token > TOKEN__INST_END) PICOCT_error_printf(&ctx, "Syntax: Unknown instruction keyword encountered");
    if (debug_mode) printf("INSTRUCTION: %s\n", token_type_names[token]);
    inst = token;
    if (inst_syntax_types[token] == IST_NONE) {}
    else if (inst_syntax_types[token] == IST_ADDR) {
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_a);
    }
    else if (inst_syntax_types[token] == IST_IMMADDR_ADDR) {
        tokenize();
        if (token != TOKEN_NUMBER && token != TOKEN_IDENTIFIER) PICOCT_error_printf(&ctx, "Expected an immediate or identifier");
        if (token == TOKEN_NUMBER) snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "#%d", token_number);
        else strncpy(temp_token_string_buffer, token_string_buffer, sizeof(temp_token_string_buffer));
        expect_identifier_exist(temp_token_string_buffer);
        hti_get(temp_token_string_buffer, &inst_a);
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_b);
    }
    else if (inst_syntax_types[token] == IST_LABEL) {
        tokenize();
        expect_token(TOKEN_LABEL_USE);
        expect_label_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_a);
    }
    else if (inst_syntax_types[token] == IST_ADDR_LABEL) {
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_a);
        tokenize();
        expect_token(TOKEN_LABEL_USE);
        expect_label_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_b);
    }
    else if (inst_syntax_types[token] == IST_LABELDEREF_ADDR) {
        tokenize();
        expect_token(TOKEN_LABEL_USE);
        expect_label_exist(token_string_buffer);
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "^%s", token_string_buffer);
        expect_label_addr_exist(temp_token_string_buffer);
        hti_get(temp_token_string_buffer, &inst_a);
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_b);
    }
    else if (inst_syntax_types[token] == IST_ADDRDEREF_ADDR) {
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "&%s", token_string_buffer);
        expect_identifier_addr_exist(temp_token_string_buffer);
        hti_get(temp_token_string_buffer, &inst_a);
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_b);
    }
    else if (inst_syntax_types[token] == IST_PORT_ADDR) {
        tokenize();
        expect_token(TOKEN_NUMBER);
        inst_a = token_number;
        tokenize();
        expect_token(TOKEN_IDENTIFIER);
        expect_identifier_exist(token_string_buffer);
        hti_get(token_string_buffer, &inst_b);
    }
    else if (inst_syntax_types[token] == IST_IMMADDR_PORT) {
        tokenize();
        if (token != TOKEN_NUMBER && token != TOKEN_IDENTIFIER) PICOCT_error_printf(&ctx, "Expected an immediate or identifier");
        if (token == TOKEN_NUMBER) snprintf(temp_token_string_buffer, sizeof(temp_token_string_buffer), "#%d", token_number);
        else strncpy(temp_token_string_buffer, token_string_buffer, sizeof(temp_token_string_buffer));
        expect_identifier_exist(temp_token_string_buffer);
        hti_get(temp_token_string_buffer, &inst_a);
        tokenize();
        expect_token(TOKEN_NUMBER);
        inst_b = token_number;
    }
    else PICOCT_error_printf(&ctx, "Syntax: Unknown instruction keyword encountered");

    size_t cisp = binary_idx;
    size_t cicp = binary_idx;
    WORD_UTYPE code_gen_a = 0, code_gen_b = 0, code_gen_c = 0;
    for (size_t i = 0; i < inst_code_gen_sizes[inst]; ++i) {
        if (debug_mode) printf("PRE CODE GEN: %zu, %zu\n", cisp, cicp);
        cicp += 3;
        switch (inst_code_gen[inst][i].a){
        case A: code_gen_a = inst_a; break;
        case B: code_gen_a = inst_b; break;
        case Z: code_gen_a = Z_ADDR; break;
        case M: code_gen_a = M_ADDR; break;
        case O: code_gen_a = O_ADDR; break;
        case P: code_gen_a = P_ADDR; break;
        case Q: code_gen_a = Q_ADDR; break;
        case R: code_gen_a = R_ADDR; break;
        case S: code_gen_a = S_ADDR; break;
        case N: code_gen_a = N_ADDR; break;
        default: code_gen_a = cisp + inst_code_gen[inst][i].a; break;
        }
        switch (inst_code_gen[inst][i].b){
        case A: code_gen_b = inst_a; break;
        case B: code_gen_b = inst_b; break;
        case Z: code_gen_b = Z_ADDR; break;
        case M: code_gen_b = M_ADDR; break;
        case O: code_gen_b = O_ADDR; break;
        case P: code_gen_b = P_ADDR; break;
        case Q: code_gen_b = Q_ADDR; break;
        case R: code_gen_b = R_ADDR; break;
        case S: code_gen_b = S_ADDR; break;
        case N: code_gen_b = N_ADDR; break;
        default: code_gen_b = cisp + inst_code_gen[inst][i].b; break;
        }
        switch (inst_code_gen[inst][i].c){
        case I: code_gen_c = cicp; break;
        case E: code_gen_c = cisp + inst_code_gen_sizes[inst] * 3; break;
        case A: code_gen_c = inst_a; break;
        case B: code_gen_c = inst_b; break;
        case N: code_gen_c = N_ADDR; break;
        default: code_gen_c = cisp + inst_code_gen[inst][i].c * 3; break;
        }
        add_inst(code_gen_a, code_gen_b, code_gen_c);
        if (debug_mode) printf("CODE GEN: %d, %d, %d\n", code_gen_a, code_gen_b, code_gen_c);
    }
    tokenize();
}

static inline void assemble(void) {
    if (debug_mode) printf("%s\n", ctx.source);
    add_binary_header();
    if (debug_mode) printf("===========DATA SECTION (variables, arrays, allocs)===========\n");
    tokenize();
    while (token != TOKEN_EOS) { if (!data_section_pass()) break; }
    if (!start_found) PICOCT_error_printf(&ctx, "__start__ symbol not found");
    if (debug_mode) printf("===========FIRST PASS (immediate collection)===========\n");
    saved_cursor = ctx.cursor;
    tokenize();
    while (token != TOKEN_EOS) { first_pass(); }
    if (debug_mode) printf("===========SECOND PASS (label collection)===========\n");
    ctx.cursor = saved_cursor;
    tokenize();
    while (token != TOKEN_EOS) { second_pass(); }
    if (debug_mode) printf("===========THIRD PASS (syntax check and code gen)===========\n");
    ctx.cursor = saved_cursor;
    tokenize();
    while (token != TOKEN_EOS) { third_pass(); }
    if (debug_mode) printf("==================================================\n");
    if (debug_mode) print_inst_costs();
    if (debug_mode) print_state();
    if (debug_mode) printf("%zu\n", binary_idx);
}

#define pop_first(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)
typedef struct {uint8_t* bytes; size_t bytes_count;} FileBytes;
static inline bool read_entire_file(const char* file_path, FileBytes* file_bytes) {
    FILE* file_handle = fopen(file_path, "rb");
    if (file_handle == NULL) return false;
    if(fseek(file_handle, 0, SEEK_END)!=0) {fclose(file_handle); return false;}
    int64_t file_size = ftell(file_handle);
    if (file_size < 0) {fclose(file_handle); return false;}
    if (file_size == 0) {file_bytes->bytes_count = 0; fclose(file_handle); return true;}
    if(fseek(file_handle, 0, SEEK_SET)!=0) {fclose(file_handle); return false;}
    file_bytes->bytes=(uint8_t*)realloc(file_bytes->bytes, (size_t)file_size);
    if(!file_bytes->bytes) {fclose(file_handle); return false;}
    file_bytes->bytes_count = fread(file_bytes->bytes, 1, (size_t)file_size, file_handle);
    bool ok = !ferror(file_handle);
    fclose(file_handle);
    return ok;
}
static inline bool write_binary(const char* file_path) {
    FILE* file = fopen(file_path, "wb");
    if (file == NULL) return false;
    size_t written = fwrite(binary, sizeof(WORD_UTYPE), binary_idx, file);
    fclose(file);
    return written == binary_idx;
}

#define MAX_PATH_LENGTH 2048
char source_file_path[MAX_PATH_LENGTH + 1] = {0};
char binary_file_path[MAX_PATH_LENGTH + 1] = {0};

int main(int argc, char** argv) {
    debug_mode = false;
    if (argc != 2) {
        printf("FATAL: Expected source file path\n");
        return 1;
    }
    pop_first(argv, argc);
    if (strlen(argv[0]) > MAX_PATH_LENGTH) {
        printf("FATAL: Source file path too long\n");
        return 1;
    }
    if (strcmp(argv[0] + strlen(argv[0]) - 4, ".sla") != 0) {
        printf("FATAL: Source file is required to be of type .sla\n");
        return 1;
    }
    strcpy(source_file_path, argv[0]);

    FileBytes file_bytes = {0};
    read_entire_file(source_file_path, &file_bytes);

    argv[0][strlen(argv[0]) - 4] = '\0';
    sprintf(binary_file_path, "%s.sq", argv[0]);

    // char* debug_source = "
    //     a = 0
    //     __start__
    //     hlt
    // ";
    // ctx.source_file_path = "debug";
    // ctx.source = debug_source;
    // ctx.source_length = strlen(debug_source);

    ctx.source_file_path = source_file_path;
    ctx.source = (char*)file_bytes.bytes;
    ctx.source_length = file_bytes.bytes_count;

    assemble();

    write_binary(binary_file_path);

    return 0;
}