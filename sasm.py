"""
zer addr
inc addr
dec addr
neg addr
add addr/imm addr
sub addr/imm addr
mul addr/imm addr
div addr/imm addr
mod addr/imm addr
not addr
and addr/imm addr
lor addr/imm addr
jez label
jlz addr label
jgz addr label
mov addr/imm
inp addr
out addr/imm
hlt
"""

trace=False
source='''
a 15

__start__

div 3 a 
add 32 a
out a
hlt           //END  
'''

def assemble(s):
    inst=[]
    var_to_val={}
    labels={}
    asm=[x.strip().split('//')[0] for x in source.split('\n') if x.strip()!='']
    start_idx = 0
    for idx, vl in enumerate(asm):
        if vl == "__start__":
            start_idx = idx + 1
            break
        var_to_val[vl.split()[0]] = int(vl.split()[1])

    n = 0
    for i in range(start_idx, len(asm)):
        tok=asm[i].split()
        if tok[0][-1]==':':
            labels[tok[0][:-1]]=n
            tok.pop(0)
        if tok[0]=='zer':
            inst.extend([tok[1], tok[1], n+3])
            n+=3
        elif tok[0]=='inc':
            inst.extend([tok[1], 'M', n+3])
            n+=3
        elif tok[0]=='dec':
            inst.extend([tok[1], 'O', n+3])
            n+=3
        elif tok[0]=='neg':
            inst.extend(['T1', tok[1], n+3, tok[1], tok[1], n+6, 'Z', 'T1', n+9, tok[1], 'Z', n+12, 'Z', 'Z', n+15, 'T1', 'T1', n+18])
            n += 18
        elif tok[0]=='add':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend(['Z', tok[1], n+3, tok[2], 'Z', n+6, 'Z', 'Z', n+9])
            n+=9
        elif tok[0]=='sub':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend([tok[2], tok[1], n+3])
            n+=3
        elif tok[0]=='mul':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend(['Z', tok[1], n+3, 'T1', 'Z', n+6, 'Z', 'Z', n+9, 'T2', tok[2], n+12, 'T1', 'O', n+15, 'T1', 'Z', n+24, tok[2], 'T2', n+21, 'Z', 'Z', n+12, 'T1', 'T1', n+27, 'T2', 'T2', n+30])
            n+=30
        elif tok[0]=='div':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend(['Z', tok[2], n+3, 'T2', 'Z', n+6, 'Z', 'Z', n+9, tok[2], tok[2], n+12, 'T2', tok[1], n+15, 'T1', 'T2', n+18, 'T1', 'Z', n+27, 'T1', 'T1', n+24, 'Z', 'Z', n+36, 'T1', 'T1', n+30, tok[2], 'M', n+33, 'Z', 'Z', n+12, 'T2', 'T2', n+39])
            n+=39
        elif tok[0]=='not':
            exit()
        elif tok[0]=='and':
            exit()
        elif tok[0]=='lor':
            exit()
        elif tok[0]=='jez':
            inst.extend([tok[1], 'Z', n+6, 'Z', 'Z', n+24, 'T1', tok[1], n+9, 'T1', 'Z', n+15, 'Z', 'Z', n+24, 'T1', 'T1', n+18, 'Z', 'Z', tok[2], 'T1', 'T1', n+27])
            n += 27
        elif tok[0]=='jlz':
            inst.extend(['T1', tok[1], n+3, 'T1', 'Z', n+12, 'T1', 'T1', n+9, 'Z', 'Z', tok[2], 'T1', 'T1', n+15])
            n += 15
        elif tok[0]=='jgz':
            inst.extend([tok[1], 'Z', n+6, 'Z', 'Z', tok[2]])
            n += 6
        elif tok[0]=='mov':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend([tok[2], tok[2], n+3, 'Z', tok[1], n+6, tok[2], 'Z', n+9, 'Z', 'Z', n+12])
            n+=12
        elif tok[0]=='inp':
            inst.extend([-1, tok[1], n+3])
            n+=3
        elif tok[0]=='out':
            try:
                var_to_val['C'+tok[1]]=int(tok[1])
                tok[1]='C'+tok[1]
            except:
                pass
            inst.extend([tok[1], -1, n+3])
            n+=3
        elif tok[0]=='hlt':
            inst.extend(['Z', 'Z', -1])
            n+=3
    for i in range(len(inst)):
        if i%3==0: 
            print()
        print(inst[i], end=",")
    print()
    for var in var_to_val:
        inst.append(var_to_val[var])
        var_to_val[var]=n
        n+=1
    inst.extend([0,0,-1,0,1])
    for i in range(len(inst)):
        if isinstance(inst[i], str):
            if inst[i][0]==':':
                inst[i]=labels[inst[i][1:]]
            elif inst[i]=='T1':
                inst[i]=len(inst)-5
            elif inst[i]=='T2':
                inst[i]=len(inst)-4
            elif inst[i]=='M':
                inst[i]=len(inst)-3
            elif inst[i]=='Z':
                inst[i]=len(inst)-2
            elif inst[i]=='O':
                inst[i]=len(inst)-1
            else:
                try:
                    inst[i]=var_to_val[inst[i]]
                except:
                    print(f"Unknown variable: {inst[i]}")
                    exit()
    return inst

import sys
def subleq(a, t):
    i = 0
    try:
        while i >= 0:
            #=============================Tracer=====================================
            if t:
                print()
                print(i)
                print(a)
                if a[i] == -1:
                    print(f"IN {a[i+1]}")
                elif a[i+1]==-1:
                    print(f"OUT a[{a[i]}]={chr(a[a[i]])}")
                else:
                    print(f"a[{a[i]}]={a[a[i]]}, a[{a[i+1]}]={a[a[i+1]]}, {a[i+2]}")
                input()
            #=======================================================================
            if a[i]==-1:
                a[a[i+1]]=ord(sys.stdin.read(1))
            elif a[i+1]==-1:
                print(f"\x1b[31m{chr(a[a[i]])}\x1b[0m", end="")
            else:
                a[a[i]] -= a[a[i+1]]
                if a[a[i]] <= 0:
                    i = a[i+2]
                    continue
            i+=3
    except (ValueError, IndexError, KeyboardInterrupt):
        print("abort")
        print(a)
program=assemble(source)
subleq(program, trace)

