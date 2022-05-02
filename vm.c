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

* Calling convention
	li r1, #addr[r3] # r1 <- 10
	li r2, #addr[r4] # r2 <- "abc=%d"

	# r1 <- 1 - printf("abc=%d", 10);
        #       2 - open
	#       3 - close
	int 80		 # syscall+r1

* Building and running the vm.
   -.1. make vm
   -.2. make run
   -.3. make clean

* RAM/ROM examples.
ROM:
0x00: 00000 111 0001 0000		-- li r1, #r2 (r2 == 0x00)
0x01: 00000 111 0010 0000
0x02: 00010 111 0001 0000
0x03: 00001 111 0001 0000
0x04: 00100 111 0001 0010
0x05: 00101 111 0001 0010
0x06: 00110 111 0001 0010
0x07: 00111 111 0010 0001
0x08: 01000 111 0000 0000
0x09: 00000 000 0000 0000

RAM:
0x00
....
0x10: 'a' <-------------------------
0x11: 'n'
0x12: 'a'
0x13: 't'
0x14: 'o'
0x15: 'l'
0x16: 'i'
0x17: 'y'
0x18: '0'
0x19:


Sample program:
00000000 0000 0000 | li r0 0       #  0 -- syscall(write)
00000000 0001 1010 | li r1 10      # 10 -- address
00000000 0010 1000 | li r2 8       #  8 -- len
00000001 0001 0000 | push r1
00000001 0010 0000 | push r2
00000010 1000 0000 | int 0x80
00000011 0000 0000 | halt


In [13]: fd=open("rom.bin", "wb")
In [14]: fd.write(b'\x00\x00\x00\x1A\x00\x28\x01\x10\x01\x20\x02\x10\x03\x00')
In [15]: fd.close()

In [1]: fd=open("ram.bin", "wb")
In [2]: fd.write(b'test\x00')
In [3]: fd.close()


--------------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>


// lab tasks:
//+ 1) написать функцию vm_print, которая выводит все регистры in hex и первые N байт ram.
//- 2) написать реализацию Control flow инструкций.
//- 3) написать реализацию Arithmetical/logical + Move инструкций.
//- 4) написать тестовые прогаммы на ассемблере.

#define ARRAY_SIZE(a) ((sizeof (a)) / (sizeof (a)[0]))
#define MAX(_x, _y) ((_x) > (_y) ? (_x) : (_y))
#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))


struct vm {
	uint64_t regs[14];
	uint64_t sp;
	uint64_t ip;

	uint16_t *rom;
	uint64_t  rom_size;

	void     *ram;
	uint64_t  ram_size;
};

static void ram_print(struct vm *vm, size_t ram_bytes_max)
{
	unsigned int i;
	for (i = 0; i < MIN(ram_bytes_max, vm->ram_size); ++i) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", ((char *) vm->ram)[i]);
	}
	printf("\n");
}

static void reg_print(struct vm *vm)
{
	unsigned int i;
	for(i = 0; i < ARRAY_SIZE(vm->regs); ++i)
		printf("r%u=0x%02lx\n", i, vm->regs[i]);
}

static void vm_print(struct vm *vm, size_t ram_bytes_max)
{
	printf("vm state:\n");
	reg_print(vm);
	ram_print(vm, ram_bytes_max);
}

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
	OP_LI,		// b00000000
	OP_PUSH,	// b00000001
	OP_INT,		// b00000010
	OP_HALT,	// b00000011
};

struct instruction {
	enum instruction_code code;
	uint8_t ra0;
	uint8_t ra1;
};

static struct instruction decode(uint16_t raw_instruction)
{
	struct instruction i = {
		.code = ((1 << 8) - 1) & (raw_instruction),
		.ra1  = ((1 << 4) - 1) & (raw_instruction >> 8),
		.ra0  = ((1 << 4) - 1) & (raw_instruction >> 12),
	};

	return i;
}
/**
* Calling convention
	li r1, #addr[r3] # r1 <- 10
	li r2, #addr[r4] # r2 <- "abc=%d"

	# r1 <- 1 - printf("abc=%d", 10);
        #       2 - open
	#       3 - close
	int 80		 # syscall+r1




	printf("%d-%d-%d", 15, 22, 01);

	stack = [
	4,
	addr("%d-%d-%d"),
	15,
	22,
	0
	]
*/

enum {
	FOO_PRINT = 1,
	FOO_OPEN = 2,
};

static bool execute(struct vm *vm, struct instruction *instruction)
{
	printf("instruction=0x%02x arg0=0x%02x arg1=0x%02x\n",
	       (int) instruction->code,
	       (int) instruction->ra0, (int) instruction->ra1);
	switch(instruction->code) {
	case OP_LI:
		vm->regs[instruction->ra0] = instruction->ra1;
		break;
	case OP_PUSH: {
		uint64_t *dst;
		vm->sp -= sizeof(uint64_t);
		dst = vm->ram + vm->sp;
		*dst = vm->regs[instruction->ra0];
		break;
	}
	case OP_INT: {
		uint64_t int_type = instruction->ra0;
		//uint64_t function = vm->regs[0];
		//uint64_t args_nr  = vm->ram[vm->sp];
		//assert(int_type == 0x80);

		// syscall write()
		{
			uint64_t *src = vm->ram + vm->sp;
			uint64_t len = src[0];
			char *buf = vm->ram + src[1];
			write(1, buf, len);
		}

		/* switch(function) { */
		/* case FOO_PRINT: */
		/* 	break; */
		/* case FOO_OPEN:  // open("filename.txt", flags) */
		/* 	/\* char *filename_addr = (char *) vm->ram[vm->sp + 1]; *\/ */
		/* 	/\* int flags = (int) vm->ram[vm->sp + 2]; *\/ */
		/* 	/\* vm->regs[16] = open(filename_addr, flags); *\/ */
		/* 	break; */
		/* default: */
		/* 	; */
		/* }; */
		break;
	}
	case OP_HALT:
	default:
		return false;
	}
	return true;
}

static int run_vm(struct vm *vm)
{
	struct instruction instruction;
	uint16_t *rom = vm->rom;
	do {
		instruction = decode(rom[vm->ip++]);
	} while(execute(vm, &instruction));

	vm_print(vm, 32);
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

		.sp = RAM_SIZE,
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
