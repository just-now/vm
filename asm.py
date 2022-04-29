# 00000000 0000 0000 | li r0 0       #  0 -- syscall(write)
# 00000000 0001 1010 | li r1 10      # 10 -- address
# 00000000 0010 1000 | li r2 8       #  8 -- len
# 00000001 0001 0000 | push r1
# 00000001 0010 0000 | push r2
# 00000010 1000 0000 | int 0x80
# 00000011 0000 0000 | halt

import sys
import re



class LI:
    def __init__(self, ai: list):
        self.ai = ai

    def emit(self):
        pass

    def print(self):
        print(self.ai, file=sys.stderr)

class PUSH:
    def __init__(self, ai: list):
        self.ai = ai

    def emit(self):
        pass

    def print(self):
        print(self.ai, file=sys.stderr)

class HALT:
    def __init__(self, ai: list):
        self.ai = ai

    def emit(self):
        pass

    def print(self):
        print(self.ai, file=sys.stderr)

class INT:
    def __init__(self, ai: list):
        self.ai = ai

    def emit(self):
        pass

    def print(self):
        print(self.ai, file=sys.stderr)


ai2ci = {
    "li":   LI,
    "push": PUSH,
    "int":  INT,
    "halt": HALT,
}

lines = sys.stdin.readlines()
ainstructions = [re.sub("#.*", "", i).split() for i in lines]
for c in ainstructions:
    ai2ci[c[0]](c).print()
