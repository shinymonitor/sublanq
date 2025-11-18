from asm import * 
from emulate import * 

source='''
a 72 69 76 76 79 32 87 79 82 76 68 0
b 0

__start__
loop: 
    drd a b
    jez b :end
    out b
    inc a
    jmp :loop
end: 
    out 10
    hlt
'''

program=assemble(source)
if program: emulate(program)
