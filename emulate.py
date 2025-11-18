import sys
def emulate(program, trace=False):
    i = 0
    try:
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
            #=======================================================================
            if program[i]==-1:
                program[program[i+1]]=ord(sys.stdin.read(1))
            elif program[i+1]==-1:
                print(f"\x1b[31m{chr(program[program[i]])}\x1b[0m", end="")
            else:
                program[program[i]] -= program[program[i+1]]
                if program[program[i]] <= 0:
                    i = program[i+2]
                    continue
            i+=3
    except (ValueError, IndexError, KeyboardInterrupt) as error:
        print(i)
        print(program)
        print(f"ERROR: {error} \nABORT")
