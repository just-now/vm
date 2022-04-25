#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

uint8_t vm_ram[1<<10];

static void load_rom_ram(const char *ram_fn)
{
	FILE *ram_fd;

	ram_fd = fopen(ram_fn, "r");
	assert(ram_fd != NULL);

	fread(vm_ram, 1, sizeof(vm_ram), ram_fd);
	assert(ferror(ram_fd) == 0);

	fclose(ram_fd);
}

int main()
{
	size_t i;
	load_rom_ram("test.bin");
	for (i = 0; i < sizeof(vm_ram); ++i) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", vm_ram[i]);
	}
	printf("\n");
	return 0;
}

/* diff -uw  <(./test) <(hexdump -C test.bin | cut -d' ' -f 3-19 | sed 's/  / /') */
/* dd if=/dev/urandom of=test.bin bs=1k count=1 */
