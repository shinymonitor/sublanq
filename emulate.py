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
        print(f"Attempted to access: program[{program[i]}] or program[{program[i+1]}]")
        print(f"Program size: {len(program)}")
    except KeyboardInterrupt:
        print(f"RUNTIME ERROR Interrupted at instruction {i}")
    except Exception as error:
        print(f"RUNTIME ERROR at instruction {i}: {error}")
        print(f"Instruction: [{program[i]}, {program[i+1]}, {program[i+2]}]")
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
