#!/usr/bin/python3

import sys
import tty
import termios
def emulate(program, trace=False):
    old_settings = termios.tcgetattr(sys.stdin)
    try:
        tty.setcbreak(sys.stdin.fileno())
        i = 0
        while i >= 0:
            #=============================Tracer=====================================
            if trace:
                print()
                print(i)
                print(program)
                if program[i] == -1:
                    print(f"IN {program[i+1]}")
                elif program[i+1]==-1:
                    print(f"OUT program[{program[i]}]={chr(program[program[i]])}")
                else:
                    print(f"program[{program[i]}]={program[program[i]]}, program[{program[i+1]}]={program[program[i+1]]}, {program[i+2]}")
                input()
            #=============================Tracer=====================================
            if program[i]==-1:
                program[program[i+1]]=ord(sys.stdin.read(1))
            elif program[i+1]==-1:
                if 0<=program[program[i]]<=127:
                    print(f"\x1b[31m{chr(program[program[i]])}\x1b[0m", end="",  flush=True)
            else:
                program[program[i]] -= program[program[i+1]]
                if program[program[i]] <= 0:
                    i = program[i+2]
                    continue
            i+=3
    except IndexError as e:
        print(f"RUNTIME ERROR at instruction {i}: Memory access out of bounds")
        if i < len(program):
            print(f"Instruction: [{program[i]}, {program[i+1] if i+1 < len(program) else '?'}, {program[i+2] if i+2 < len(program) else '?'}]")
        print(f"Program size: {len(program)}")
    except KeyboardInterrupt:
        print(f"RUNTIME ERROR Interrupted at instruction {i}")
    except Exception as error:
        print(f"RUNTIME ERROR at instruction {i}: {error}")
        print(f"Instruction: [{program[i]}, {program[i+1]}, {program[i+2]}]")
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

if __name__ == '__main__':
    import sys
    if len(sys.argv)<2 or sys.argv[1][-3:]!='.sq':
        print("INCORRECT ARGUMENTS\nUsage: emulate.py <file.sq>")
        sys.exit(1)
    try:
        with open(sys.argv[1], 'r') as f:
            program = list(map(int, f.read().split()))
    except:
        print("Failed to open", sys.argv[1])
        sys.exit(1)
    emulate(program, trace='--trace' in sys.argv)