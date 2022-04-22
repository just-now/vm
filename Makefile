vm:
	gcc -std=c99 -O0 -g -Wall -Wextra vm.c -o vm
run:
	./vm vm.c vm.c
clean:
	rm -f vm
