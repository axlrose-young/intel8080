all: main.c
	gcc -O2 -Wall -Wextra -o emu main.c bdos.c cpu.c debugger.c
clean: 
	rm -f emu

