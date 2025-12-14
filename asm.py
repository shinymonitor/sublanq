#!/usr/bin/python3

def assemble(source, debug=False):
    program=[]
    vvm={}
    labels={}
    source=[x for x in [x.split('//')[0].strip() for x in source.split('\n')] if x!='']
    try:
        data, instructions=source[:source.index("__start__")], source[source.index("__start__")+1:]
    except:
        print("ASSEMBLER ERROR: Must have a __start__ symbol")
        return False
    for l in data:
        if l.split()[0] in ['Z', 'M', 'O', 'T1', 'T2'] or l.split()[0][0]=='C' or ':' in l.split()[0]:
            print(f"ASSEMBLER ERROR: Invalid variable name: {l}")
            return False
        if l.split()[0] in vvm:
            print(f"ASSEMBLER ERROR: Reassignment of varaible: {l}")
            return False
        try:
            if len(l.split())>2:
                vvm[l.split()[0]] = [int(x) for x in l.split()[1:]]
            elif len(l.split())==2:
                vvm[l.split()[0]] = int(l.split()[1])
            else:
                print(f"ASSEMBLER ERROR: Variable must have at least 1 assigned value: {l}")
                return False
        except:
            print(f"ASSEMBLER ERROR: Invalid variable assignment: {l}")
            return False
    n = 0
    for instruction in instructions:
        inst=instruction.split()
        if inst[0][-1]==':':
            labels[inst[0][:-1]]=n
            inst.pop(0)
            if inst==[]:
                continue
        if inst[0] in ['zer', 'inc', 'dec', 'neg', 'jmp'] and len(inst)!=2 or inst[0] in ['add', 'sub', 'mul', 'div', 'mod', 'mov', 'drd', 'dwt', 'jez', 'jlz', 'jle', 'inp', 'out'] and len(inst)!=3 or inst[0]=='hlt' and len(inst)!=1:
            print(f"ASSEMBLER ERROR: Incorrect number of arguments given: {instruction}")
            return False
        if inst[0] in ['zer', 'inc', 'dec', 'neg', 'add', 'sub', 'mul', 'div', 'mod', 'drd', 'dwt'] and inst[-1] not in vvm:
            print(f"ASSEMBLER ERROR: Unknown variable: {instruction}")
            return False
        if inst[0] in ['jez', 'jlz', 'jle', 'inp'] and inst[1] not in vvm:
            print(f"ASSEMBLER ERROR: Unknown variable: {instruction}")
            return False
        if inst[0] in ['jmp', 'jez', 'jlz', 'jle'] and inst[-1][0] != ':':
            print(f"ASSEMBLER ERROR: Label must start with ':': {instruction}")
            return False
        if inst[0] in ['add', 'sub', 'mul', 'div', 'mod', 'mov', 'dwt', 'out'] and inst[1] not in vvm and not (inst[1].isdigit() or inst[1].startswith('-') and inst[1][1:].isdigit()):
            print(f"ASSEMBLER ERROR: Invalid variable or immediate: {instruction}")
            return False
        if inst[0] in ['inp', 'out'] and not inst[2].isdigit():
            print(f"ASSEMBLER ERROR: Invalid port (must be a non-negative immediate): {instruction}")
            return False

        if inst[0] in ['add', 'sub', 'mul', 'div', 'mod', 'mov', 'dwt', 'out'] and (inst[1].isdigit() or inst[1].startswith('-') and inst[1][1:].isdigit()):
            vvm['C'+inst[1]]=int(inst[1])
            inst[1]='C'+inst[1]
        if inst[0] in ['inp', 'out']:
            inst[2]=int(inst[2])

        if inst[0]=='zer':
            program.extend([inst[1], inst[1], n+3])
            n+=3
        elif inst[0]=='inc':
            program.extend([inst[1], 'M', n+3])
            n+=3
        elif inst[0]=='dec':
            program.extend([inst[1], 'O', n+3])
            n+=3
        elif inst[0]=='neg':
            program.extend(['T1', inst[1], n+3, inst[1], inst[1], n+6, 'Z', 'T1', n+9, inst[1], 'Z', n+12, 'Z', 'Z', n+15, 'T1', 'T1', n+18])
            n += 18
        elif inst[0]=='add':
            program.extend(['Z', inst[1], n+3, inst[2], 'Z', n+6, 'Z', 'Z', n+9])
            n+=9
        elif inst[0]=='sub':
            program.extend([inst[2], inst[1], n+3])
            n+=3
        elif inst[0]=='mul':
            program.extend(['Z', inst[1], n+3, 'T1', 'Z', n+6, 'Z', 'Z', n+9, 'T2', inst[2], n+12, 'T1', 'O', n+15, 'T1', 'Z', n+24, inst[2], 'T2', n+21, 'Z', 'Z', n+12, 'T1', 'T1', n+27, 'T2', 'T2', n+30])
            n+=30
        elif inst[0]=='div':
            program.extend(['Z', inst[2], n+3, 'T2', 'Z', n+6, 'Z', 'Z', n+9, inst[2], inst[2], n+12, 'T2', inst[1], n+15, 'T1', 'T2', n+18, 'T1', 'Z', n+27, 'T1', 'T1', n+24, 'Z', 'Z', n+36, 'T1', 'T1', n+30, inst[2], 'M', n+33, 'Z', 'Z', n+12, 'T2', 'T2', n+39])
            n+=39
        elif inst[0]=='mod':
            program.extend([inst[2], inst[1], n+3, 'T1', inst[2], n+6, 'T1', 'Z', n+21, 'T1', 'T1', n+12, 'Z', inst[1], n+15, inst[2], 'Z', n+18, 'Z', 'Z', n+27, 'T1', 'T1', n+24, 'Z', 'Z', n])
            n+=27
        elif inst[0]=='jmp':
            program.extend(['Z', 'Z', inst[1]])
            n+=3
        elif inst[0]=='jle':
            program.extend([inst[1], 'Z', inst[2]])
            n+=3
        elif inst[0]=='jlz':
            program.extend(['T1', inst[1], n+3, 'T1', 'Z', n+12, 'T1', 'T1', n+9, 'Z', 'Z', inst[2], 'T1', 'T1', n+15])
            n+=15
        elif inst[0]=='jez':
            program.extend([inst[1], 'Z', n+6, 'Z', 'Z', n+24, 'T1', inst[1], n+9, 'T1', 'Z', n+15, 'Z', 'Z', n+21, 'T1', 'T1', n+18, 'Z', 'Z', inst[2], 'T1', 'T1', n+24])
            n+=24
        elif inst[0]=='mov':
            program.extend([inst[2], inst[2], n+3, 'Z', inst[1], n+6, inst[2], 'Z', n+9, 'Z', 'Z', n+12])
            n+=12
        elif inst[0]=='drd':
            program.extend([n+16, n+16, n+3, 'Z', inst[1], n+6, n+16, 'Z', n+9, 'Z', 'Z', n+12, inst[2], inst[2], n+15, 'Z', 0, n+18, inst[2], 'Z', n+21, 'Z', 'Z', n+24])
            n+=24
        elif inst[0]=='dwt':
            program.extend([n+24, n+24, n+3, n+25, n+25, n+6, n+30, n+30, n+9, 'Z', inst[2], n+12, n+24, 'Z', n+15, n+25, 'Z', n+18, n+30, 'Z', n+21, 'Z', 'Z', n+24, 0, 0, n+27, 'Z', inst[1], n+30, 0, 'Z', n+33, 'Z', 'Z', n+36])
            n+=36
        elif inst[0]=='inp':
            program.extend([-1, inst[1], inst[2]])
            n+=3
        elif inst[0]=='out':
            program.extend([inst[1], -1, inst[2]])
            n+=3
        elif inst[0]=='hlt':
            program.extend(['Z', 'Z', -1])
            n+=3
        else:
            print(f"ASSEMBLER ERROR: Invalid operation: {instruction}")
            return False
    if debug:
        print(program, '\n', vvm, '\n', labels)
    for v in vvm:
        temp=n
        if isinstance(vvm[v], int):
            program.append(vvm[v])
        else:
            program.append(n+1)
            for x in vvm[v]:
                program.append(x)
            n+=len(vvm[v])
        vvm[v]=temp
        n+=1
    program.extend([0,0,-1,0,1])
    res_var=['T1', 'T2', 'M', 'Z', 'O']
    for i in range(len(program)):
        if isinstance(program[i], str):
            if program[i][0]==':':
                try:
                    program[i]=labels[program[i][1:]]
                except:
                    print(f"ASSEMBLER ERROR: Unknown label: {program[i][1:]}")
                    return False
            elif program[i] in res_var:
                program[i]=len(program)-(len(res_var)-res_var.index(program[i]))
            else:
                program[i]=vvm[program[i]]
    if debug:
        print(program, '\n', vvm)
    return program

if __name__ == '__main__':
    import sys
    if len(sys.argv)<2 or sys.argv[1][-4:]!='.sla':
        print("INCORRECT ARGUMENTS\nUsage: asm.py <file.sla>")
        sys.exit(1)
    try:
        with open(sys.argv[1], 'r') as f:
            source = f.read()
    except:
        print("Failed to open", sys.argv[1])
        sys.exit(1)
    program = assemble(source, debug='--debug' in sys.argv)
    if program:
        with open(''.join(sys.argv[1].split('.')[:-1])+'.sq', 'w') as f:
            f.write(' '.join(map(str, program)))
