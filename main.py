from asm import * 

EXAMPLES = {\
"hello_world": '''
msg 72 69 76 76 79 32 87 79 82 76 68 0
i 0
__start__
loop: drd msg i
    jle i :end
    out i
    inc msg
    jmp :loop
end: hlt
''',\
"guess_game": '''
a 0
b 55
c 3
temp 0
__start__
loop:
inp a
out a
mov a temp
sub b temp
jez temp :win
out 88
out 10
dec c
jle c :lose
jmp :loop
lose:
out 76
out 10
hlt
win:
out 87
out 10
hlt
'''
}

program=assemble(EXAMPLES['hello_world'])
if program: emulate(program)


