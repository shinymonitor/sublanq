#ifndef PICOCT_H
#define PICOCT_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct {
    size_t i, x, y, l;
} PICOCT_Cursor;

typedef enum {
    PICOCT_MATCH,
    PICOCT_IDENTIFIER,
    PICOCT_NUMBER,
    PICOCT_EOS,
    PICOCT_UNKNOWN,
} PICOCT_TokenType;

typedef struct {
    const char* match_str;
    size_t match_type;
} PICOCT_Match;

typedef struct {
    const char* source_file_path;
    char* source;
    size_t source_length;

    PICOCT_Cursor cursor;
    PICOCT_Cursor old_cursor;

    const char* comment_marker;
    PICOCT_Match* keyword_match_table;
    size_t keyword_match_table_count;
    PICOCT_Match* symbol_match_table;
    size_t symbol_match_table_count;

    PICOCT_TokenType token_type;
    size_t match_type;
    char* token_string_buffer;
    size_t token_string_capacity;
    size_t token_string_length;
    size_t token_number;
} PICOCT_Context;

static inline void PICOCT_append_token_char(PICOCT_Context* ctx, char c) {
    if (ctx->token_string_length + 1 < ctx->token_string_capacity) ctx->token_string_buffer[ctx->token_string_length++] = c;
}

static inline void PICOCT_new_token(PICOCT_Context* ctx) {
    ctx->token_string_buffer[0] = '\0';
    ctx->token_string_length = 0;
    ctx->token_number = 0;
    ctx->old_cursor = ctx->cursor;
}

static inline void PICOCT_advance_cursor(PICOCT_Context* ctx) {
    if (ctx->cursor.i >= ctx->source_length) return;
    if (ctx->source[ctx->cursor.i] == '\n') {
        ctx->cursor.x = 0;
        ++ctx->cursor.y;
        ctx->cursor.l = ctx->cursor.i + 1;
    } 
    else ++ctx->cursor.x;
    ++ctx->cursor.i;
}
static inline void PICOCT_advance_cursor_n(PICOCT_Context* ctx, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (ctx->cursor.i >= ctx->source_length) return;
        if (ctx->source[ctx->cursor.i] == '\n') {
            ctx->cursor.x = 0;
            ++ctx->cursor.y;
            ctx->cursor.l = ctx->cursor.i + 1;
        } 
        else ++ctx->cursor.x;
        ++ctx->cursor.i;
    }
}
static inline void PICOCT_until_non_whitespace(PICOCT_Context* ctx) {
    while (ctx->cursor.i < ctx->source_length && isspace(ctx->source[ctx->cursor.i])) PICOCT_advance_cursor(ctx);
}
static inline void PICOCT_until_newline(PICOCT_Context* ctx) {
    while (ctx->cursor.i < ctx->source_length && ctx->source[ctx->cursor.i] != '\n') PICOCT_advance_cursor(ctx);
}

static inline bool PICOCT_is_keyword(PICOCT_Context* ctx, const char* keyword_str) {
    size_t len = strlen(keyword_str);
    if (ctx->cursor.i + len > ctx->source_length) return false;
    if (memcmp(ctx->source + ctx->cursor.i, keyword_str, len) == 0) {
        if (ctx->cursor.i + len < ctx->source_length && (isalnum(ctx->source[ctx->cursor.i + len]) || ctx->source[ctx->cursor.i + len] == '_')) return false;
        PICOCT_advance_cursor_n(ctx, len);
        PICOCT_until_non_whitespace(ctx);
        return true;
    }
    return false;
}
static inline bool PICOCT_is_symbol(PICOCT_Context* ctx, const char* symbol_str) {
    size_t len = strlen(symbol_str);
    if (ctx->cursor.i + len > ctx->source_length) return false;
    if (memcmp(ctx->source + ctx->cursor.i, symbol_str, len) == 0) {
        PICOCT_advance_cursor_n(ctx, len);
        PICOCT_until_non_whitespace(ctx);
        return true;
    }
    return false;
}

static inline void PICOCT_tokenize(PICOCT_Context* ctx) {
    PICOCT_new_token(ctx); PICOCT_until_non_whitespace(ctx);
    if (ctx->cursor.i >= ctx->source_length) { ctx->token_type = PICOCT_EOS; return; }
    while (PICOCT_is_symbol(ctx, ctx->comment_marker)) {
        PICOCT_until_newline(ctx);
        PICOCT_until_non_whitespace(ctx);
    }
    if (ctx->cursor.i >= ctx->source_length) { ctx->token_type = PICOCT_EOS; return; }
    for (size_t i = 0; i < ctx->symbol_match_table_count; ++i) {
        if (PICOCT_is_symbol(ctx, ctx->symbol_match_table[i].match_str)) {
            ctx->token_type = PICOCT_MATCH;
            ctx->match_type = ctx->symbol_match_table[i].match_type;
            return;
        }
    }
    if (ctx->cursor.i >= ctx->source_length) { ctx->token_type = PICOCT_EOS; return; }
    for (size_t i = 0; i < ctx->keyword_match_table_count; ++i) {
        if (PICOCT_is_keyword(ctx, ctx->keyword_match_table[i].match_str)) {
            ctx->token_type = PICOCT_MATCH;
            ctx->match_type = ctx->keyword_match_table[i].match_type;
            return;
        }
    }
    if (isalpha(ctx->source[ctx->cursor.i]) || ctx->source[ctx->cursor.i] == '_') {
        while (ctx->cursor.i < ctx->source_length && (isalnum(ctx->source[ctx->cursor.i]) || ctx->source[ctx->cursor.i] == '_')) {
            PICOCT_append_token_char(ctx, ctx->source[ctx->cursor.i]);
            PICOCT_advance_cursor(ctx);
        }
        PICOCT_append_token_char(ctx, '\0');
        PICOCT_until_non_whitespace(ctx);
        ctx->token_type = PICOCT_IDENTIFIER;
        return;
    }
    else if (isdigit(ctx->source[ctx->cursor.i])) {
        while (ctx->cursor.i < ctx->source_length && isdigit(ctx->source[ctx->cursor.i])) {
            PICOCT_append_token_char(ctx, ctx->source[ctx->cursor.i]);
            PICOCT_advance_cursor(ctx);
        }
        PICOCT_append_token_char(ctx, '\0');
        PICOCT_until_non_whitespace(ctx);
        ctx->token_type = PICOCT_NUMBER;
        ctx->token_number = strtoul(ctx->token_string_buffer, NULL, 10);
        return;
    }
    ctx->token_type = PICOCT_UNKNOWN; 
    return;
}

static inline void PICOCT_error_printf(PICOCT_Context* ctx, const char* fmt, ...) {
    printf("%s:%zu:%zu: ", ctx->source_file_path, ctx->old_cursor.y + 1, ctx->old_cursor.x + 1);
    va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    va_end(args);
    printf("\n ");
    size_t la = 0;
    size_t cnt = 0;
    while (ctx->old_cursor.l + la < ctx->source_length && ctx->source[ctx->old_cursor.l + la] != '\n') {
        if (ctx->source[ctx->old_cursor.l + la] == '\t') printf(" ");
        else printf("%c", ctx->source[ctx->old_cursor.l + la]);
        if (cnt < ctx->old_cursor.x) cnt += 1;
        ++la;
    }
    printf("\n ");
    for (size_t i = 0; i < cnt; ++i) printf(" ");
    printf("^\n");
    exit(1);
}

#endif // PICOCT_H