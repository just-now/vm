# 00000000 0000 0000 | li r0 0       #  0 -- syscall(write)
# 00000000 0001 1010 | li r1 10      # 10 -- address
# 00000000 0010 1000 | li r2 8       #  8 -- len
# 00000001 0001 0000 | push r1
# 00000001 0010 0000 | push r2
# 00000010 1000 0000 | int 0x80
# 00000011 0000 0000 | halt

   # -.1. Instruction set.
   #      -.1.1. Arithmetic/logic;
   #             add, sub,
   #             addw, subw,

   #             and, or, not, xor,
   #             andw, orw, notw, xorw,

   #             asr, asl       # arithmetical shift left/right

   #      -.1.2. Move;
   #             l,s, lw,sw, li
   #             push, pop, pushw, popw

   #      -.1.3. Control flow;
   #              -.1.3.0. j, jne, je,   call, ret
   #      	         # call ADDRx    # push PC+1, PC <- ADDRx
   #      	-.1.3.1. Interrupts: hw; # ignored for now
   #      	-.1.3.2. Interrupts: sw; # FFI
   #      -.1.4. Service instructions;
   #              HALT

import sys
import re


definitions = {
    "li"	: '00',
    "push"	: '01',
    "int"	: '02',
    "halt"	: '03',
    "add"	: '04',
    "sub"	: '05',
    "addw"	: '06',
    "subw"	: '07',
    "and"	: '08',
    "or"	: '09',
    "not"	: '0a',
    "xor"	: '0b',
    "andw"	: '0c',
    "orw"	: '0d',
    "notw"	: '0e',
    "xorw"	: '0f',
    "r0"  	: '0',
    "r1"  	: '1',
    "r2"  	: '2',
    "r3"  	: '3',
    "r4"  	: '4',
    "r5"  	: '5',
    "r6"  	: '6',
    "r7"  	: '7',
    "r8"  	: '8',
    "r9"  	: '9',
    "r10" 	: 'a',
    "r11" 	: 'b',
    "r12" 	: 'c',
    "r13" 	: 'd',
    "sp"  	: 'e',
    "ip"  	: 'f',
}

lines = sys.stdin.readlines()
ainstructions = [re.sub("#.*", "", i).split() for i in lines]
assmed1 = ["".join([definitions[x] if x in definitions
                    else x for x in i])
    for i in ainstructions]
def trailer(x: str):
    assert(len(x) <= 4)
    s = x+(4-len(x))*"0"
    return bytes.fromhex(s[0:2])+bytes.fromhex(s[2:4])

assmed2 = [trailer(a) for a in assmed1]

assmed3=b''
for a in assmed2:
    assmed3 += a

with open("rom1.bin", "wb") as fd:
    fd.write(assmed3)

