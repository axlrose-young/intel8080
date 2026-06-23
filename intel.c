#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define DEBUG 0

bool is_running = 0;

typedef struct {
	uint8_t memory[0xffff];
	uint16_t pc;

	// work registers
	uint8_t a,b,c,d,e,h,l; 

	//flags
	bool sf, zf, pf, cy, ac;

	uint16_t sp;
	
	bool INTE;
}chip;

void chip_init(chip* c)
{
	memset(c->memory,0,0xffff);
	c->a = 0;
	c->b = 0;
	c->c = 0;
	c->d = 0;
	c->e = 0
	c->h = 0;
	c->l = 0;
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

void unformat_flags(uint8_t byte, chip* c){
	c->sf = (byte & 0x80) >> 7;
	c->zf = (byte & 0x40) >> 6;
	c->ac = (byte & 0x10) >> 4;
	c->pf = (byte & 0x04) >> 2;
	c->cy = byte & 0x01;
}

// Flags handling
void handle_zf(uint8_t result,chip* c)
{
	c->zf = (result == 0); 
}

void handle_sf(uint8_t result,chip* c)
{
	c->sf = (result >> 7);
}

void handle_pf(uint8_t result, chip* c)
{
	uint8_t ones = 0;
	for(uint8_t i=0; i < 8; i++){
		if((result & (1 << i)) != 0){
			ones++;	
		}
	}
	c->pf = (ones % 2 == 0);
}

void handle_zsp(uint8_t result, chip* c){
	handle_zf(result, c);	
	handle_sf(result, c);
	handle_pf(result, c);
}

// Instruction families
void ora(uint8_t val, chip* c){
	c->a |= val;
	c->cy = c->ac = 0;
	handle_zsp(c->a,c);
}

void xra(uint8_t val, chip* c)
{
	c->a ^= val;
	c->cy = c->ac = 0;
	handle_zsp(c->a,c);
}

void logical_and(uint8_t val, chip* c){
	c->cy = 0;
	c->ac = (((c->a | val)&8) != 0);	
	c->a &= val;
	handle_zsp(c->a,c);
}

void push_stack(uint8_t high, uint8_t low, chip* c){
	c->sp--;
	c->memory[c->sp] = high;
	c->sp--;
	c->memory[c->sp] = low;
}

uint16_t pop_stack(chip* c){
	uint8_t low = c->memory[c->sp];
	c->sp++;
	uint8_t high = c->memory[c->sp];
	c->sp++;
	return (high << 8) | low;
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
		handle_ac_add(c->reg[index],0x01,c);
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

uint8_t dcr(uint8_t val, chip* c){
	c->ac = (val&0x0f) >= (0x01);
	val--;
	handle_zsp(val,c);	
	return val;
}

void dcr_pair(uint8_t* high, uint8_t* low, chip* c){
	uint16_t val = (*high << 8) | *low;
	val--;
	*high = val >> 8;	
	*low = val & 0xff;
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

void cmp(uint8_t val, chip* c){
	uint8_t result = c->a - val;
	c->ac = (c->a & 0x0f) >= (val & 0x0f);
	c->cy = (c->a < val);
	handle_zsp(result,c);
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
		case 0x7c: c->a = c->h; c->pc++; break;
		case 0x7d: c->a = c->l; c->pc++; break; 
		case 0x78: c->a = c->b; c->pc++; break;
		case 0x79: c->a = c->c; c->pc++; break;
		case 0x6f: c->l = c->a; c->pc++; break;
		case 0x5f: c->e = c->a; c->pc++; break;
		case 0x54: c->d = c->h; c->pc++; break;
		case 0x5d: c->e = c->l; c->pc++; break;
		case 0x7b: c->a = c->e; c->pc++; break;
		case 0x4f: c->c = c->a; c->pc++; break;
		case 0x7a: c->a = c->d; c->pc++; break;
		case 0x47: c->b = c->a; c->pc++; break;
		case 0x69: c->l = c->c; c->pc++; break;

		case 0x7e: c->a = c->memory[(c->h << 8) | c->l]; c->pc++; break; 
		case 0x66: c->h = c->memory[(c->h << 8) | c->l]; c->pc++; break; 
		case 0x5e: c->e = c->memory[(c->h << 8) | c->l]; c->pc++; break;
 		case 0x46: c->b = c->memory[(c->h << 8) | c->l]; c->pc++; break;
 		case 0x4e: c->c = c->memory[(c->h << 8) | c->l]; c->pc++; break;

		case 0x77: c->memory[(c->h << 8) | c->l] = c->a; c->pc++; break;

		// MVI commands
		case 0x3e: c->a = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x06: c->b = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x26: c->h = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x0e: c->c = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x16: c->d = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x36: c->memory[(c->h << 8) | c->l] = c->memory[c->pc+1]; 
			   c->pc+=2;
			   break;

		// ORA commands
		case 0xb6: ora(c->memory[(c->h<<8)|c->l],c); c->pc++; break;
		case 0xb1: ora(c->c,c); c->pc++; break;

		// XRA commands
		case 0xae: xra(c->memory[(c->h<<8)|c->l],c); c->pc++; break;
		case 0xa8: xra(c->b,c); c->pc++; break;
		case 0xa9: xra(c->c,c); c->pc++; break;
		
		// LOAD commands (LXI)
		case 0x01: lxi(opcode,c); c->pc+=3; break;
		case 0x11: lxi(opcode,c); c->pc+=3; break;
		case 0x21: lxi(opcode,c); c->pc+=3; break;
		case 0x31: lxi(opcode,c); c->pc+=3; break;

		// Loading to memory from accumulator
		case 0x3a: c->a = c->memory[make_addr(c)]; c->pc+=3; break; 		// lda addr
		case 0x32: c->memory[make_addr(c)] = c->a; c->pc+=3; break;		// sta addr
										
		case 0x12: c->memory[(c->d<<8) | c->e] = c->a; c->pc++; break;		// stax

		case 0x1a: c->a = c->memory[(c->d<<8) | c->e]; c->pc++; break;		// ldax

		case 0x2a:								// lhld 
			c->l = c->memory[make_addr(c)];		
			c->h = c->memory[make_addr(c)+1];	 
			c->pc+=3; break;

		case 0x22: 								// shld
			c->memory[make_addr(c)] = c->l;
			c->memory[make_addr(c)+1] = c->h;
			c->pc+=3; break;

		case 0xf9: c->sp = (c->h<<8)|c->l; c->pc++; break;			// sphl

		// cmp commands
		case 0xfe: cmp(c->memory[c->pc+1], c); c->pc+=2; break;			// cpi

		// RET Addr
		case 0xc9: c->pc = pop_stack(c); break;
		case 0xd8: c->pc = (c->cy == 1) ? pop_stack(c) : c->pc+1; break;
		case 0xd0: c->pc = (c->cy == 0) ? pop_stack(c) : c->pc+1; break;
		case 0xe8: c->pc = (c->pf == 1) ? pop_stack(c) : c->pc+1; break;
		case 0xc0: c->pc = (c->zf == 0) ? pop_stack(c) : c->pc+1; break;
		case 0xf8: c->pc = (c->sf == 1) ? pop_stack(c) : c->pc+1; break;
		case 0xf0: c->pc = (c->sf == 0) ? pop_stack(c) : c->pc+1; break;
		
		// JMP
		case 0xc3: c->pc = make_addr(c); break;					// jmp	
		case 0xca: c->pc = (c->zf == 1) ? make_addr(c) : c->pc+3; break;	// jz
		case 0xc2: c->pc = (c->zf == 0) ? make_addr(c) : c->pc+3; break;  	// jnz 
		case 0xda: c->pc = (c->cy == 1) ? make_addr(c) : c->pc+3; break; 	// jc
		case 0xd2: c->pc = (c->cy == 0) ? make_addr(c) : c->pc+3; break;   	// jnc
		case 0xea: c->pc = (c->pf == 1) ? make_addr(c) : c->pc+3; break;	// jpe
		case 0xe2: c->pc = (c->pf == 0) ? make_addr(c) : c->pc+3; break;	// jpo
		case 0xfa: c->pc = (c->sf == 1) ? make_addr(c) : c->pc+3; break;	// jm
		case 0xf2: c->pc = (c->sf == 0) ? make_addr(c) : c->pc+3; break;	// jp
		case 0xe9: c->pc = (c->h<<8) | c->l; break;				// pchl

			
		// CALL Addr
		case 0xcd:			
			push_stack((c->pc+3) >> 8,(c->pc+3) & 0xff, c);
			c->pc = make_addr(c); 
			break;
		case 0xdc:
			if(c->cy){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);
			}
			else { c->pc += 3; } 
			break;

		case 0xd4:
			if(!c->cy){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);	
			}
			else { c->pc += 3; } 
			break;
		case 0xec:
			if(c->pf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);	
			}
			else { c->pc+=3; }
			break;
		case 0xe4:
			if(!c->pf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);	
			}
			else { c->pc+=3; }
			break;
		case 0xcc:
			if(c->zf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);	
			}
			else { c->pc+=3; }
			break;
		case 0xc4:
			if(!c->zf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);	
			}
			else { c->pc+=3; }
			break;
		case 0xfc:
			if(c->sf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);
			}
			else { c->pc+=3; }
			break;
		case 0xf4:
			if(!c->sf){
				push_stack((c->pc+3)>>8,(c->pc+3) & 0xff, c); 
				c->pc = make_addr(c);
			}
			else { c->pc+=3; }
			break;


		// PUSH commands
		case 0xe5: push_stack(c->h,c->l,c); c->pc++; break;		// push h,l
		case 0xd5: push_stack(c->d,c->e,c); c->pc++; break; 		// push d,e	
		case 0xc5: push_stack(c->b,c->c,c); c->pc++; break;		// push b,c
		case 0xf5: push_stack(c->a,format_flags(c), c); c->pc++; break; // push psw
			   	
		// POP commands
		case 0xc1: 							// pop b,c
			  uint16_t bc = pop_stack(c); 
			  c->b = bc >> 8; c->c = (be & 0xff);
			  c->pc++; break; 
		case 0xd1:							// pop d,e
			  uint16_t de = pop_stack(c); 
			  c->d = de >> 8; c->e = (de & 0xff);
			  c->pc++; break; 
		case 0xe1:							// pop h,l
			  uint16_t hl = pop_stack(c); 
			  c->h = hl >> 8; c->l = (hl & 0xff);
			  c->pc++; break; 
		case 0xf1:							// pop psw
			uint16_t psw = pop_stack(c);	  
			c->a = psw >> 8;
			unformat_flags(psw,c);
			c->pc++;
			break;

		// AND operations
		case 0xe6: logical_and(c->memory[c->pc+1],c); c->pc+=2; break;  // ani
		case 0xa1: logical_and(c->c,c); c->pc++; break;			// ana c
		case 0xa0: logical_and(c->b,c); c->pc++; break;			// ana b
	
		// rotate
		case 0x0f:							// rrc
			c->cy = c->a & 1;
			c->a >>= 1;
			c->a |= (c->cy << 7);	
			c->pc++;
			break; 

		// DCR commands
		case 0x05: c->b = dcr(c->b,c); c->pc++; break;			// dcr b
		case 0x0d: c->d = dcr(c->d,c); c->pc++; break;			// dcr c
					
		// DCX commands
		case 0x2b: dcr_pair(&c->h,&c->l,c); c->pc++; break;		// dcx h
		case 0x0b: dcr_pair(&c->b,&c->c,c); c->pc++; break;		// dcx b
									
		// INR commands
		case 0x3c: return increment(opcode,c);
		case 0x14: return increment(opcode,c);
		case 0x34: return increment(opcode,c);

		
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
			printf("Unimplemented: %x\n",opcode);
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
		execute(&c);	
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
			fflush(stdout);
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
