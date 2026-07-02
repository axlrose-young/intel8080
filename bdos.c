#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

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
			break;
		case 2:
			printf("%c",c->e);
			fflush(stdout);
			break;
		default:
			printf("unimplemented bdos: %d\n",c->c);
			c->is_running = 0;
			break;
	}
	c->pc = pop(c);
}
