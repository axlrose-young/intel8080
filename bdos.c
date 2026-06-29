#include <stdio.h>
#include "cpu.h"

static inline uint16_t get_de(chip* c){
	return (c->d << 8) | c->e;
}

static uint16_t pop(chip* c){
	uint8_t low = c->memory[c->sp];
	c->sp++;
	uint8_t high = c->memory[c->sp];
	c->sp++;
	return (high << 8) | low;
}

void mini_bdos(chip* c)
{ 
	switch(c->c)
	{
		case 9:
			uint16_t addr = get_de(c);	
			while(c->memory[addr] != '$')
			{
				printf("%c",c->memory[addr]);	
				addr++;
			}
			fflush(stdout);

			// RET to addr after CALL 5
			c->pc = pop(c);
			break;
		case 2:
			printf("%c",c->e);
			fflush(stdout);
			c->pc = pop(c);
			break;
		default:
			printf("unimplemented bdos: %d\n",c->c);
			break;
	}
}
