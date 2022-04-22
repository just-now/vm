/**
--------------------------------------------------------------------------------

* Data flow.
                  py-slisp                       ./vm (vm.c)
   (lisp source) -----------> (IROM) ------------------------------------> RESULT
   (def foo(a b c d) ...)      [ opcode arg0 arg1 arg2 ...               console output+
   (foo 1 2 3 4)                 lw     r1   #r2  # r1 <- MEM[#r2]        calculations.
			         00001 0001 0010 --> 0x112
				 sw    #r2   r1   # MEM[#r2] <- r1
			       ... ]

* General moving parts of the vm.

   IROM                    VM core                Data memory
0+------------+     +-----------------------+   0+--------------+
 | lw r1 #r2  |     |                       |    |
 | sw #r2 r1  <=====|IP (address) (inc/set) |    |
 |            fetch |                   #SP |====>
 |            |=====> decode/execute        | #r2|
 |            |     |                   #r2 |====> r1
 |                  +A----------------------+    |
0xAAAA               |        |         |       0XFFFF
	timer--------+	      V         |
                   FFI (foreign function|interface)
                           +-------+    V
			   |	   | interrupts
			   |   ?   |  +----+
			   +-------+  |    |
				      +----+
* Instruction memory segement.
   -.1. Instruction set.
	-.1.1. Arithmetic/logic;
	       add, sub,
	       addw, subw,

	       and, or, not, xor,
	       andw, orw, notw, xorw,

	       asr, asl       # arithmetical shift left/right

	-.1.2. Move;
	       l,s, lw,sw, li
	       push, pop, pushw, popw

	-.1.3. Control flow;
	        -.1.3.0. j, jne, je,   call, ret
		         # call ADDRx    # push PC+1, PC <- ADDRx
		-.1.3.1. Interrupts: hw; # ignored for now
		-.1.3.2. Interrupts: sw; # FFI
	-.1.4. Service instructions;
	        HALT

   -.2. Instruction format.
   0		            15
   |------------------------|
    OPCODE(8) REG1(4) REG2(4)


    OPCODE
    0b00000000  - add
    0b00000001  - sub
    0b00000010  - addw
    0b00000011  - subw
...
    REGADDR
    0b0000	- R0
    0b0001	- R1
...
    0b1111	- IP
    0b1110	- SP
...

   -.3. Memory map.
    +-------------------------------------------------+
  i0| j 0xAA
  i1| j 0x100500 # --> void timer_int() { counter++; }
  i2| j
  i3|
 .i.|
  iA|
 ...|
 pAA|  lw ... # program start
 .p.|
 .p.|
100500 sw ... # timer_int
 .p.|

* Data memory segement.
   -.1. Memory map.
   0+------------------------------+
    |            .BSS(consts)      |
    +------------------------------+
    |            .DATA             |
   H+------------------------------+
    |		   |		   |
    |		   V		   |
    |            .HEAP             |
   ...				  ...
    +--------------A---------------+
    |              |               |
    |            .STACK            |
FFFF+------------------------------+

   -.2. Granularity.

* Data/Instruction memory and other structures geometry.
 IROM - 4 GB
 DM   - 4 GB
 REGS - 64Bit
 ADDR - 32Bit

* Building and running the vm.
   -.1. make vm
   -.2. make run
   -.3. make clean
--------------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <stdbool.h>

struct vm {
	uint64_t regs[16];
	uint64_t sp;
	uint64_t ip;

	void     *rom;
	uint64_t  rom_size;

	void     *ram;
	uint64_t  ram_size;
};

static void usage()
{
	printf("./vm rom_file ram_file\n");
}

static int load_rom_ram(struct vm *vm, const char *ram_fn, const char *rom_fn)
{
	FILE *ram_fd;
	FILE *rom_fd;
	int   rc;

	ram_fd = fopen(ram_fn, "r");
	rom_fd = fopen(rom_fn, "r");

	if (ram_fd == NULL || rom_fd == NULL) {
		usage();
		return -ENOENT;
	}

	rc = 0;
	fread(vm->ram, 1, vm->ram_size, ram_fd);
	fread(vm->rom, 1, vm->rom_size, rom_fd);

	if (ferror(ram_fd) != 0 || ferror(rom_fd) != 0) {
		rc = -EBADF;
		printf("Unable to read rom or ram files\n");
	}

	fclose(ram_fd);
	fclose(rom_fd);
	return rc;
}

static int parse_args(int argc, char **argv, char **ram_fn, char **rom_fn)
{
	if (argc != 3)
		return -EINVAL;

	*rom_fn = argv[1];
	*ram_fn = argv[2];
	return 0;
}

enum instruction_code {
	OP_ADD,
	OP_ADDW,
	OP_JUMP,
};

struct instruction {
	enum instruction_code code;
	uint8_t regaddr0;
	uint8_t regaddr1;
};

static struct instruction decode(uint16_t raw_instruction)
{
	struct instruction i = {};
	return i;
}

static bool execute(struct vm *vm, struct instruction *instruction)
{
	switch(instruction->code) {
	case OP_ADDW:
		vm->regs[instruction->regaddr0] += vm->regs[instruction->regaddr1];
		break;
	case OP_JUMP:
		vm->ip = vm->regs[instruction->regaddr0];
		break;
	case OP_HALT:
	default:
		return false;
	}
	return true;
}

static void vm_print(struct vm *vm)
{
	printf("vm state:\n");
}

static int run_vm(struct vm *vm)
{
	struct instruction instruction;
	uint16_t *rom = vm->rom;
	do {
		instruction = decode(rom[vm->ip++]);
	} while(execute(vm, &instruction));

	vm_print(vm);
	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	char *ram_fn;
	char *rom_fn;

	const uint64_t ROM_SIZE = 64 * (1 << 10);
	const uint64_t RAM_SIZE = 64 * (1 << 10);

	struct vm vm = {
		.rom = calloc(ROM_SIZE, sizeof(char)),
		.rom_size = ROM_SIZE,

		.ram = calloc(RAM_SIZE, sizeof(char)),
		.ram_size = RAM_SIZE,
	};

	rc = parse_args(argc, argv, &ram_fn, &rom_fn);
	if (rc != 0) {
		usage();
		return -rc;
	}

	rc = load_rom_ram(&vm, ram_fn, rom_fn);
	if (rc != 0)
		return -rc;

	return run_vm(&vm);
}
