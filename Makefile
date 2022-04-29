vm: vm.c
	gcc -std=c99 -O0 -g -Wall -Wextra vm.c -o vm
run:
	./vm vm.c vm.c
clean:
	rm -f vm test rom1.bin

test: test.c
	gcc -std=c99 -O0 -g -Wall -Wextra test.c -o test

run_test: test
	dd if=/dev/urandom of=test.bin bs=1k count=1
	bash -c 'diff -uw  <(./test) <(hexdump -C test.bin | cut -d" " -f 3-19 | sed "s/  / /")'

asm1:
	python3 asm.py < asm1.prg > rom1.bin
