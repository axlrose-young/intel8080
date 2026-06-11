#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define DEBUG 1
bool is_running = 0;

typedef struct {
	unsigned char memory[0xffff];
	uint16_t pc;

	// work registers
	unsigned char reg[8];	

	//flags
	unsigned char sf, zf, pf, cy, ac;

	uint16_t sp;
}chip;

void chip_init(chip* c)
{
	memset(c->memory,0,0xffff);
	memset(c->reg,0,7);
	c->pc = 0;
	c->sf = 0;
	c->zf = 0;
	c->pf = 0;
	c->cy = 0;
	c->ac = 0;
	c->sp = 0;

	FILE* pfile = fopen("8080PRE.COM","rb");
	if(pfile == NULL)
	{
		perror("Error opening file");	
	}
	// loading at 0x100
	fread(&c->memory[0x100],0xffff,1,pfile);	

	c->pc = 0x100;
	is_running = 1;
}

// Helper functions

void handle_zf(chip* c)
{
	uint8_t result = c->reg[7] - (c->memory[c->pc + 1]); 
	if(result == 0) { c->zf = 1; }
}

void handle_cy(chip* c)
{
	if(c->reg[7] < (c->memory[c->pc+1])) { c->cy = 1; }
}

void handle_sf(chip* c)
{
	(opcode >> 7 == 1)? c->sf = 1 : c->sf = 0; 
}

void mvi_reg(uint16_t opcode, chip* c)
{
	c->reg[(opcode>>3)&7] = c->memory[c->pc+1];	
	c->pc+=2;
}
	

int execute(chip* c)
{	
	// condition to exit loop
	if(c->pc > 0xffff)
	{
		is_running = 0;	
	}

	uint8_t opcode = c->memory[c->pc];

	// basic disassembler 
	if(DEBUG)
	{
		printf("Opcode: %x, PC: %x\n",opcode, c->pc);	
		printf("Registers  B: %x C: %x D: %x E: %x H: %x L: %x M: %x A: %x\n",
				c->reg[0],c->reg[1],c->reg[2],c->reg[3],
			       c->reg[4], c->reg[5],c->reg[6],c->reg[7]);	
		printf("Flags  Z: %d S: %d P: %d CY: %d AC: %d\n",c->zf,c->sf,c->pf,c->cy,c->ac);
		printf("\t---\n");
	}

	switch(opcode)
	{
		case 0x3e: mvi_reg(opcode, c); return 7;
		case 0xfe:
			handle_zf(c);	
			handle_cy(c);
			handle_sf(c);
			c->pc+=2;
			return 7;
		default: 	
			is_running = 0;
			return 0;
	}
}

int main()
{
	// initialize the chip
	chip c;	
	unsigned int states = 0;
	
	// load the chip with the rom
	chip_init(&c);

	while(is_running)
	{
		states+=execute(&c);	
	}
	return 0;
}
