.PHONY:all debug clean

CC=gcc
CFLAGS= -Wall -Wextra -O2 
DEBUG= -Wall -Wextra -O0 -g

all: emu

emu: main.c cpu.c bdos.c debugger.c
	$(CC) $(CFLAGS) $^ -o $@

debug: main.c cpu.c bdos.c debugger.c
	$(CC) $(DEBUG) $^ -o emu

clean: 
	rm -f emu

