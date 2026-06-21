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
	
	bool  INTE;
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
	c->INTE = 0;

	FILE* pfile = fopen("8080EXM.COM","rb");
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

// Making PSW low byte
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

void handle_ac_sub(uint8_t value1, uint8_t value2, chip* c)
{
	c->ac = (value1 < value2);
}

void handle_ac_add(uint8_t value1, uint8_t value2,chip* c)
{
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

int ora(uint8_t opcode, chip* c)
{
	uint8_t src = opcode & 7;
	if(src != 6)
	{
		c->reg[7] |= c->reg[src];
		handle_pf(c->reg[7],c);
		handle_sf(c->reg[7],c);
		handle_zf(c->reg[7],c);
		c->cy = c->ac = 0;	
		c->pc+=1;
		return 4;
	}
	else 
	{
		uint16_t addr = (c->reg[4] << 8) | c->reg[5];
		c->reg[7] |= c->memory[addr];	
		handle_pf(c->reg[7],c);
		handle_sf(c->reg[7],c);
		handle_zf(c->reg[7],c);
		c->cy = c->ac = 0;
		c->pc+=1;
		return 7;
	}
}

int xra(uint8_t opcode, chip* c)
{
	uint8_t src = opcode & 7;
	if(src != 6)
	{
		c->reg[7] ^= c->reg[src];
		handle_pf(c->reg[7],c);
		handle_sf(c->reg[7],c);
		handle_zf(c->reg[7],c);
		c->cy = c->ac = 0;	
		c->pc+=1;
		return 4;
	}
	else 
	{
		uint16_t addr = (c->reg[4] << 8) | c->reg[5];
		c->reg[7] ^= c->memory[addr];	
		handle_pf(c->reg[7],c);
		handle_sf(c->reg[7],c);
		handle_zf(c->reg[7],c);
		c->cy = c->ac = 0;
		c->pc+=1;
		return 7;
	}
}


int ana(uint8_t opcode, chip* c)
{
	uint8_t index = opcode & 7;	
	if(index != 6)
	{
		handle_ac_add((c->reg[7] & 0x0f), (c->reg[index] & 0x0f), c);
		c->reg[7] &= c->reg[index];
		handle_pf(c->reg[7],c);		
    		handle_sf(c->reg[7],c);		
		handle_zf(c->reg[7],c);		
  
		c->cy = 0;
		c->pc+=1;
		return 4;
	}
	else
	{
		uint16_t addr = (c->reg[4] << 8) | c->reg[5];	
		handle_ac_add((c->reg[7] & 0x0f), (c->memory[addr] & 0x0f), c);
		c->reg[7] &= c->memory[addr];
		handle_pf(c->reg[7],c);		
    		handle_sf(c->reg[7],c);		
		handle_zf(c->reg[7],c);		
  
		c->cy = 0;
		c->pc+=1;
		return 7;
	}
}

int increment(uint8_t opcode, chip* c)
{
	uint8_t index = (opcode >> 3) & 7; 
	if(index != 6)
	{
		c->reg[index]++;	
		handle_zf(c->reg[index],c);
		handle_sf(c->reg[index],c);
		handle_pf(c->reg[index],c);
		handle_ac_sub(c->reg[index],0x01,c);
		c->pc+=1;
		return 5;
	}
	else 
	{
		uint16_t addr = (c->reg[4] << 8) | c->reg[5];	
		c->memory[addr]++;
		handle_zf(c->memory[addr],c);
		handle_sf(c->memory[addr],c);
		handle_pf(c->memory[addr],c);
		handle_ac_add(c->memory[addr], 0x01,c);
		c->pc+=1;
		return 10;
	}
}

void increment_rp(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 0xff;
	switch(rp)
	{
		case 0: 
			{
				uint16_t val = (c->reg[0]<<8) | c->reg[1]; 
				val++;
				c->reg[0] = val >> 8;
				c->reg[1] = val & 0xff;
				break;
			}
		case 1: 
			{
				uint16_t val = (c->reg[2]<<8) | c->reg[3]; 
				val++;
				c->reg[2] = val >> 8;
				c->reg[3] = val & 0xff;
				break;
			}
		case 2: 
			{
				uint16_t val = (c->reg[4]<<8) | c->reg[5]; 
				val++;
				c->reg[4] = val >> 8;
				c->reg[5] = val & 0xff;
				break;	
			}
		case 3: 
			c->sp++; 
			break;
	}
}

int decrement(uint8_t opcode, chip* c)
{
	uint8_t index = (opcode >> 3) & 7;
	if(index != 6)
	{
		c->reg[index]--;	
		handle_zf(c->reg[index],c);
		handle_sf(c->reg[index],c);
		handle_pf(c->reg[index],c);
		handle_ac_sub(c->reg[index],0x01,c);
		c->pc+=1;
		return 5;
	}
	else 
	{
		uint16_t addr = (c->reg[4] << 8) | c->reg[5];	
		c->memory[addr]--;
		handle_zf(c->memory[addr],c);
		handle_sf(c->memory[addr],c);
		handle_pf(c->memory[addr],c);
		handle_ac_sub(c->memory[addr], 0x01,c);
		c->pc+=1;
		return 10;
	}
}

void decrement_rp(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 0xff;
	switch(rp)
	{
		case 0: 
			{
				uint16_t val = (c->reg[0]<<8) | c->reg[1]; 
				val--;
				c->reg[0] = val >> 8;
				c->reg[1] = val & 0xff;
				break;
			}
		case 1: 
			{
				uint16_t val = (c->reg[2]<<8) | c->reg[3]; 
				val--;
				c->reg[2] = val >> 8;
				c->reg[3] = val & 0xff;
				break;
			}
		case 2: 
			{
				uint16_t val = (c->reg[4]<<8) | c->reg[5]; 
				val--;
				c->reg[4] = val >> 8;
				c->reg[5] = val & 0xff;
				break;	
			}
		case 3: 
			c->sp--; 
			break;
	}

}

void lxi(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 3;
	switch(rp)
	{
		case 0:	
			c->reg[0] = c->memory[c->pc+2];
			c->reg[1] = c->memory[c->pc+1];
			break;
		case 1:
			c->reg[2] = c->memory[c->pc+2];
			c->reg[3] = c->memory[c->pc+1];
			break;
		case 2:
			c->reg[4] = c->memory[c->pc+2];
			c->reg[5] = c->memory[c->pc+1];
			break;
		case 3:
			c->sp = make_addr(c);
			break;
	}
}

void dad(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 3;
	uint16_t hl_rp = (c->reg[4] << 8) | c->reg[5];
	uint16_t value2;
	switch(rp)
	{
		case 0: value2 = (c->reg[0] << 8) | c->reg[1]; break;
		case 1: value2 = (c->reg[2] << 8) | c->reg[3]; break;
		case 2: value2 = (c->reg[4] << 8) | c->reg[5]; break;
		case 3: value2 = c->sp; break;
	}
	uint16_t result = hl_rp + value2; 	// if result overflow it wraps around to make a smaller value
	if(result < hl_rp) { c->cy = 1; }
	else { c->cy = 0; }
	
	c->reg[4] = (result >> 8);
	c->reg[5] = (result & 0xff);
}

void debug(uint8_t opcode,chip* c);
void mini_bdos(chip* c);

int execute(chip* c)
{	
	// condition to exit loop
	if(c->pc > 0xffff)
	{
		is_running = 0;	
	}
	
	// MINI BDOS STUB
	if(c->pc == 5)
	{
		mini_bdos(c);	
	}

	uint8_t opcode = c->memory[c->pc];

	// basic disassembler 
	if(DEBUG) { debug(opcode,c); }

	switch(opcode)
	{
		// MOV commands
		case 0x7c: mov_to_reg(opcode, c); return 5;
		case 0x7d: mov_to_reg(opcode, c); return 5;
		case 0x78: mov_to_reg(opcode, c); return 5;
		case 0x79: mov_to_reg(opcode, c); return 5;
		case 0x6f: mov_to_reg(opcode, c); return 5;
		case 0x5f: mov_to_reg(opcode, c); return 5;
		case 0x54: mov_to_reg(opcode, c); return 5;
		case 0x5d: mov_to_reg(opcode, c); return 5;
		case 0x7b: mov_to_reg(opcode, c); return 5;
		case 0x4f: mov_to_reg(opcode, c); return 5;
		case 0x7a: mov_to_reg(opcode, c); return 5;
		case 0x47: mov_to_reg(opcode, c); return 5;
		case 0x69: mov_to_reg(opcode, c); return 5;

		case 0x7e: mov_from_mem(opcode,c); return 7; 
		case 0x66: mov_from_mem(opcode,c); return 7; 
		case 0x5e: mov_from_mem(opcode,c); return 7; 
		case 0x77: mov_from_mem(opcode,c); return 7; 		
		case 0x46: mov_from_mem(opcode,c); return 7; 
		case 0x4e: mov_from_mem(opcode,c); return 7;

		// MVI commands
		case 0x3e: mvi_reg(opcode, c); return 7;
		case 0x06: mvi_reg(opcode, c); return 7;
		case 0x26: mvi_reg(opcode, c); return 7;
		case 0x0e: mvi_reg(opcode, c); return 7;
		case 0x16: mvi_reg(opcode, c); return 7;
		case 0x36: 
			   c->memory[(c->reg[4] << 8) | c->reg[5]] = c->memory[c->pc+1];
			   c->pc+=2;
			   return 10;

		// ORA commands
		case 0xb6: return ora(opcode,c);
		case 0xb1: return ora(opcode,c);

		// XRA commands
		case 0xae: return xra(opcode,c);
		case 0xa8: return xra(opcode,c); 
		case 0xa9: return xra(opcode,c);
	
		// ANA
		case 0xa1: return ana(opcode,c);
		case 0xa0: return ana(opcode,c);

		// LOAD commands (LXI)
		case 0x01: lxi(opcode,c); c->pc+=3; return 10;
		case 0x11: lxi(opcode,c); c->pc+=3; return 10;
		case 0x21: lxi(opcode,c); c->pc+=3; return 10;
		case 0x31: lxi(opcode,c); c->pc+=3; return 10;

		// Loading to memory from accumulator
		case 0x3a: c->reg[7] = c->memory[make_addr(c)]; c->pc+=3; return 13;
		case 0x32: c->memory[make_addr(c)] = c->reg[7]; c->pc+=3; return 13;
		case 0x12: 
			   c->memory[(c->reg[2] << 8) | c->reg[3]] = c->reg[7];
			   c->pc+=1;
			   return 7;
		case 0x1a:
			   c->reg[7] = c->memory[(c->reg[2] << 8) | c->reg[3]];
			   c->pc+=1;
			   return 7;
		// LHLD
		case 0x2a:
			c->reg[5] = c->memory[make_addr(c)];		// reg L
			c->reg[4] = c->memory[make_addr(c) + 1];	// reg H 
			c->pc+=3;
			return 16;

		// SHLD
		case 0x22: 
			c->memory[make_addr(c)] = c->reg[5];
			c->memory[make_addr(c)+1] = c->reg[4];
			c->pc+=3;
			return 16;

		// SPHL
		case 0xf9:	
			c->sp = (c->reg[4] << 8) | c->reg[5];
			c->pc+=1;
			return 5;

		case 0xfe:
			{
			uint8_t result = c->reg[7] - c->memory[c->pc + 1];
			uint8_t value1 = c->reg[7] & 0x0f;
			uint8_t value2 = c->memory[c->pc+1] & 0x0f;
			handle_zf(result,c);	
			handle_sf(result,c);
			handle_pf(result,c);
			handle_cy(c);
			handle_ac_sub(value1,value2,c);

			c->pc+=2;
			return 7;
			}
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

		// CALL if/not carry
		case 0xdc:
			if(c->cy)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;		
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc += 3; return 11; } 

		case 0xd4:
			if(!c->cy)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;		
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;	
			}
			else { c->pc += 3; return 11; } 
		case 0xec:
			if(c->pf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }
		case 0xe4:
			if(!c->pf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }
		case 0xcc:
			if(c->zf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }
		case 0xc4:
			if(!c->zf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }
		case 0xfc:
			if(c->sf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }
		case 0xf4:
			if(!c->sf)
			{
				c->sp--;
				c->memory[c->sp] = (c->pc+3) >> 8;	
				c->sp--;
				c->memory[c->sp] = (c->pc+3) & 0xff;
				c->pc = make_addr(c);
				return 17;
			}
			else { c->pc+=3; return 11; }

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

		// RET if/not carry
		case 0xd8:
		       if(c->cy)
		       {
		      		uint8_t low = c->memory[c->sp]; 
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;
				return 11;
		       }	       
		       else { c->pc+=1; return 5; }

		case 0xd0:
			if(!c->cy)
		       {
		      		uint8_t low = c->memory[c->sp]; 
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;
				return 11;
		       }	       
		       else { c->pc+=1; return 5; }
		case 0xe8:
			if(c->pf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		case 0xe0:
			if(!c->pf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		case 0xc8:
			if(c->zf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		case 0xc0:
			if(!c->zf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		case 0xf8:
			if(c->sf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		case 0xf0:
			if(!c->sf)
			{
				uint8_t low = c->memory[c->sp];
				c->sp++;
				uint8_t high = c->memory[c->sp];
				c->sp++;
				c->pc = (high << 8) | low;	
				return 11;
			}
			else { c->pc+=1; return 5; }
		// JMP if/not carry
		case 0xda:
			if(c->cy) { c->pc = make_addr(c); }
		       	else { c->pc+=1; }	
			return 10;
		case 0xd2:
			if(!c->cy) { c->pc = make_addr(c); }
		       	else { c->pc+=1; }	
			return 10;

		case 0xea:
			if(c->pf) { c->pc = make_addr(c); }
			else { c->pc+=3; }
			return 10;
		case 0xe2:
			if(!c->pf) { c->pc = make_addr(c); }
			else { c->pc+=3; }
			return 10;
		case 0xfa:
			if(c->sf) { c->pc = make_addr(c); }
			else { c->pc+=3; }
			return 10;
		case 0xf2:
			if(!c->sf) { c->pc = make_addr(c); }
			else { c->pc+=3; }
			return 10;

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

		case 0xe9:				// PCHL
			c->pc = (c->reg[4] << 8) | c->reg[5];
			return 5;

		case 0xe6:
			{
				c->reg[7] &= c->memory[c->pc+1];
				c->cy = 0;
				uint8_t value1 = c->reg[7] & 0x0f;
				uint8_t value2 = c->memory[c->pc+1] & 0x0f;
				handle_zf(c->reg[7],c);
				handle_sf(c->reg[7],c);
				handle_pf(c->reg[7],c);
				handle_ac_add(value1,value2,c);

				c->pc+=2;
				return 7;
			}

		case 0x0f:
			c->cy = c->reg[7] & 1;
			c->reg[7] >>= 1;
			c->reg[7] |= (c->cy << 7);	
			c->pc+=1;
			return 4; 

		// DCR commands
		case 0x05: return decrement(opcode,c);
		case 0x0d: return decrement(opcode,c);
									
		// INR commands
		case 0x3c: return increment(opcode,c);
		case 0x14: return increment(opcode,c);
		case 0x34: return increment(opcode,c);

		// DCX commands
		case 0x2b: decrement_rp(opcode,c); c->pc+=1; return 5;
		case 0x0b: decrement_rp(opcode,c); c->pc+=1; return 5;

		// INX commands
		case 0x23: increment_rp(opcode,c); c->pc+=1; return 5;
		case 0x13: increment_rp(opcode,c); c->pc+=1; return 5;
						
		case 0x09: dad(opcode,c); c->pc+=1; return 10;
		case 0x19: dad(opcode,c); c->pc+=1; return 10;
		case 0x29: dad(opcode,c); c->pc+=1; return 10;
		case 0x39: dad(opcode,c); c->pc+=1; return 10;
		case 0xeb: 
			   uint16_t hl_rp = (c->reg[4] << 8) | c->reg[5];
			   uint16_t de_rp = (c->reg[2] << 8) | c->reg[3];
			   uint16_t copy = hl_rp;
			   hl_rp = de_rp;
			   de_rp = copy;
			   c->reg[4] = hl_rp >> 8;
			   c->reg[5] = hl_rp & 0xff;
			   c->reg[2] = de_rp >> 8;
			   c->reg[3] = de_rp & 0xff;

			   c->pc+=1;
			   return 4;

		case 0x07:
			c->cy = (c->reg[7] >> 7);
			c->reg[7] <<= 1;
			c->reg[7] |= c->cy;

			c->pc+=1;
			return 4;

		case 0xf3: c->INTE = 0; c->pc+=1; return 4;
		case 0xfb: c->INTE = 1; c->pc+=1; return 4;

		case 0x00: c->pc+=1; return 4;
		case 0x37: c->cy = 1; c->pc+=1; return 4;
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
	fprintf(stderr,"states: %d\n",states);
	return 0;
}

void debug(uint8_t opcode,chip* c)
{
	fprintf(stderr,"Opcode: %x, PC: %x\n",opcode, c->pc);	
	fprintf(stderr,"Registers  B: %x C: %x D: %x E: %x H: %x L: %x M: %x A: %x\n",
			c->reg[0],c->reg[1],c->reg[2],c->reg[3],
		       c->reg[4], c->reg[5],c->reg[6],c->reg[7]);	
	fprintf(stderr,"Flags  Z: %d S: %d P: %d CY: %d AC: %d Stack Ptr: %x\n",c->zf,c->sf,c->pf,c->cy,c->ac,c->sp);
	fprintf(stderr,"\t---\n");
}

void mini_bdos(chip* c)
{
	switch(c->reg[1])
	{
		case 9:
			c->pc = (c->reg[2] << 8) | c->reg[3];	
			while(c->memory[c->pc] != '$')
			{
				printf("%c",c->memory[c->pc]);	
				c->pc+=1;
			}
			// RET to addr after CALL 5
			uint8_t low = c->memory[c->sp]; 			
			c->sp++;
			uint8_t high = c->memory[c->sp];
			c->sp++;
			c->pc = (high << 8) | low;
			break;
		default:
			printf("unimplemented bdos: %d\n",c->reg[1]);
			break;
	}
}
