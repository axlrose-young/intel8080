 #include <stdio.h>
#include <stdint.h>
#include <string.h>
#define DEBUG 1

bool is_running = 0;

typedef struct {
	uint8_t memory[0xffff];
	uint16_t pc;

	// work registers
	uint8_t reg[8];	

	//flags
	uint8_t sf, zf, pf, cy, ac;

	uint16_t sp;
}chip;

void chip_init(chip* c)
{
	memset(c->memory,0,0xffff);
	memset(c->reg,0,8);
	c->pc = 0;
	c->sf = 0;
	c->zf = 0;
	c->pf = 0;
	c->cy = 0;
	c->ac = 0;
	c->sp = 0xffff;	

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

void rp_push(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 3;
	switch(rp)
	{	
		case 0: 
			c->sp--;
			c->memory[c->sp] = c->reg[0];
			c->sp--;
			c->memory[c->sp] = c->reg[1];
			break;
		case 1:  
			c->sp--;
			c->memory[c->sp] = c->reg[2];
			c->sp--;
			c->memory[c->sp] = c->reg[3];
			break;
		case 2: 
			c->sp--;
			c->memory[c->sp] = c->reg[4];
			c->sp--;
			c->memory[c->sp] = c->reg[5];
			break;
	}
}

void rp_pop(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 3;
	switch(rp)
	{	
		case 0: 
			c->reg[1] = c->memory[c->sp];	 // resgiter C
			c->sp++;
			c->reg[0] = c->memory[c->sp];	 // resgiter B
			c->sp++;			
			break;
		case 1:  
			c->reg[3] = c->memory[c->sp];	 // register E 
			c->sp++;
			c->reg[2] = c->memory[c->sp];	 // register D
			c->sp++;
			break;
		case 2: 
			c->reg[5] = c->memory[c->sp];	 // register L 
			c->sp++;
			c->reg[4] = c->memory[c->sp];	 // register H
			c->sp++;
			break;
	}
}

uint16_t make_addr(chip* c)
{
	return c->memory[c->pc + 2] << 8 | c->memory[c->pc + 1];
}

uint8_t format_flags(chip* c)
{
	uint8_t low = 0x00;	
	low |= c->sf << 7;
	low |= c->zf << 6;
	low |= c->ac << 4;
        low |= c->pf << 2;
	low |= 0x02;
	low |= c->cy;	
	
	return low;
}

void handle_zf(uint8_t result,chip* c)
{
	c->zf = (result == 0); 
}

void handle_cy(chip* c)
{
	c->cy = (c->reg[7] < c->memory[c->pc+1]);
}

void handle_sf(uint8_t result,chip* c)
{
	c->sf = (result >> 7);
}

void handle_pf(uint8_t result, chip* c)
{
	result ^= result >> 4;
	result ^= result >> 2;
	result ^= result >> 1;
	c->pf = !(result & 1);		
}

void handle_ac_sub(chip* c)
{
	uint8_t value1 = c->reg[7] & 0x0f;
	uint8_t value2 = c->memory[c->pc+1] & 0x0f;
	c->ac = (value1 < value2);
}

void handle_ac_add(chip* c)
{
	uint8_t value1 = c->reg[7] & 0x0f;
	uint8_t value2 = c->memory[c->pc+1] & 0x0f;
	c->ac = ((value1 + value2) > 0x0f)? 1: 0;
}

void mvi_reg(uint16_t opcode, chip* c)
{
	c->reg[(opcode>>3)&7] = c->memory[c->pc+1];	
	c->pc+=2;
}

void mov_to_reg(uint8_t opcode,chip* c)
{
	uint8_t dest = (opcode >> 3) & 7;
	uint8_t src = opcode & 7;

	if(dest != src) { c->reg[dest] = c->reg[src]; }
	c->pc+=1;
}

void mov_from_mem(uint8_t opcode, chip* c)
{
	uint8_t index = (opcode >> 3) & 7;
	uint16_t data = c->reg[4] << 8 | c->reg[5];
	c->reg[index] = c->memory[data];
	c->pc+=1;
}

void debug(uint8_t opcode,chip* c);

int execute(chip* c)
{	
	// condition to exit loop
	if(c->pc > 0xffff)
	{
		is_running = 0;	
	}

	uint8_t opcode = c->memory[c->pc];

	// basic disassembler 
	if(DEBUG) { debug(opcode,c); }

	switch(opcode)
	{
		// MOV commands
		case 0x7c: mov_to_reg(opcode, c); return 5;
		case 0x7d: mov_to_reg(opcode, c); return 5;
		case 0x7e: mov_from_mem(opcode,c); return 7;

		
		// MVI commands
		case 0x3e: mvi_reg(opcode, c); return 7;

		// LOAD commands
		case 0x21:
			uint16_t data = make_addr(c);
			c->reg[4] = (data & 0xff00)>>8;	
			c->reg[5] = (data & 0xff);
			c->pc+=3;
			return 10;
		case 0x31: c->sp = make_addr(c); c->pc+=3; return 10;
		case 0x3a: c->reg[7] = c->memory[make_addr(c)]; c->pc+=3; return 13;
			
		case 0xfe:
			uint8_t result = c->reg[7] - c->memory[c->pc + 1];

			handle_zf(result,c);	
			handle_sf(result,c);
			handle_pf(result,c);
			handle_cy(c);
			handle_ac_sub(c);

			c->pc+=2;
			return 7;
		case 0xca:
			(c->zf) ? (c->pc = make_addr(c)) : (c->pc += 3);
			return 10;
		case 0xc2:
			!(c->zf) ? (c->pc = make_addr(c)) : (c->pc += 3);
			return 10;
		case 0xc3: c->pc = make_addr(c); return 10;	

		// CALL Addr
		case 0xcd:			
			c->sp--;
			c->memory[c->sp] = ((c->pc+3)>>8) & 0xff;
			c->sp--;
			c->memory[c->sp] = ((c->pc+3)) & 0xff;

			c->pc = make_addr(c);
			return 17;
		// RET Addr
		case 0xc9:
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;
				return 10;
			}

		// PUSH commands
		case 0xe5: rp_push(opcode,c); c->pc+=1; return 11;
		case 0xd5: rp_push(opcode,c); c->pc+=1; return 11;	
		case 0xc5: rp_push(opcode,c); c->pc+=1; return 11;	
		case 0xf5:
			   uint8_t low = format_flags(c);
			   uint8_t high = c->reg[7];
			   c->sp--;
			   c->memory[c->sp] = high;
			   c->sp--;
			   c->memory[c->sp] = low;

			   c->pc+=1;
			   return 11;
		
		// POP commands
		case 0xc1: rp_pop(opcode,c); c->pc+=1; return 10;
		case 0xd1: rp_pop(opcode,c); c->pc+=1; return 10;
		case 0xe1: rp_pop(opcode,c); c->pc+=1; return 10;

		case 0xf1:				 // POP PSW
			uint8_t flags = c->memory[c->sp];	
			c->sf = (flags & 0x80) >> 7;
			c->zf = (flags & 0x40) >> 6;
			c->ac = (flags & 0x10) >> 4;
			c->pf = (flags & 0x04) >> 2;
			c->cy = flags & 0x01;

			c->sp++;
			c->reg[7] = c->memory[c->sp]; 
			c->sp++;

			c->pc+=1;
			return 10;

		case 0xe6:
			c->reg[7] &= c->memory[c->pc+1];
			c->cy = 0;
			handle_zf(c->reg[7],c);
			handle_sf(c->reg[7],c);
			handle_pf(c->reg[7],c);
			handle_ac_add(c);

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

void debug(uint8_t opcode,chip* c)
{
	printf("Opcode: %x, PC: %x\n",opcode, c->pc);	
	printf("Registers  B: %x C: %x D: %x E: %x H: %x L: %x M: %x A: %x\n",
			c->reg[0],c->reg[1],c->reg[2],c->reg[3],
		       c->reg[4], c->reg[5],c->reg[6],c->reg[7]);	
	printf("Flags  Z: %d S: %d P: %d CY: %d AC: %d Stack Ptr: %x\n",c->zf,c->sf,c->pf,c->cy,c->ac,c->sp);
	printf("\t---\n");
}
