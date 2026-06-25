// work on push stack implement get register pair commands

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
	c->e = 0;
	c->h = 0;
	c->l = 0;
	c->pc = 0;
	c->sf = 0;
	c->zf = 0;
	c->pf = 0;
	c->cy = 0;
	c->ac = 0;
	c->sp = 0;	
	c->INTE = 0;

	FILE* pfile = fopen("8080EXM.COM","rb");
	if(pfile == NULL)
	{
		perror("Error opening file");	
	}
	// loading at 0x100
	fread(&c->memory[0x100],0xffff,1,pfile);	

	c->memory[6] = 0x01;
	c->memory[7] = 0xc9;
	c->pc = 0x100;
	is_running = 1;
}

// Helper functions

/*
void mem_write(uint8_t val, uint16_t addr, chip* c){
	fprintf(stderr,"PC: %04X, Written val: %02X, to addr: %04X\n",c->pc,val,addr);
}
*/

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

// Getting resgister pairs

uint16_t get_bc(chip* c){
	return (c->b<<8) | c->c;
}

uint16_t get_de(chip* c){
	return (c->d<<8) | c->e;
}

uint16_t get_hl(chip* c){
	return (c->h<<8) | c->l;
}

// Instruction families
void ora(uint8_t val, chip* c){
	c->a |= val;
	c->cy = c->ac = 0;
	handle_zsp(c->a,c);
}

void add(uint8_t val, chip* c){
	c->ac = (((c->a&0x0f) + (val&0x0f)) > 0x0f);
	c->a += val;
	handle_zsp(c->a,c);	
	c->cy = (c->a < val);
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

uint8_t inr(uint8_t val, chip* c){
	c->ac = ((val&0x0f) + 0x01) > 0x0f; 
	val++;
	handle_zsp(val,c);
	return val;
}

void inr_pair(uint8_t* high, uint8_t* low, chip* c){
	uint16_t val = (*high << 8) | *low;
	val++;
	*high = val >> 8;	
	*low = val & 0xff;
}

void lxi(uint8_t opcode, chip* c)
{
	uint8_t rp = (opcode >> 4) & 3;
	switch(rp)
	{
		case 0:	
			c->b = c->memory[c->pc+2];
			c->c = c->memory[c->pc+1];
			break;
		case 1:
			c->d = c->memory[c->pc+2];
			c->e = c->memory[c->pc+1];
			break;
		case 2:
			c->h = c->memory[c->pc+2];
			c->l = c->memory[c->pc+1];
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

void dad(uint16_t val, chip* c){
	uint16_t hl = get_hl(c);
	hl += val;
	c->cy = (hl < val);			// result if overflows wraps around
	c->h = hl >> 8;
	c->l = hl & 0xff;
}

void debug(uint8_t opcode,chip* c);
void mini_bdos(chip* c);

int execute(chip* c)
{	
	// condition to exit loop
	if(c->pc > 0xffff) { is_running = 0; }
	
	// MINI BDOS STUB
	if(c->pc == 5) { mini_bdos(c); }

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

		case 0x7e: c->a = c->memory[get_hl(c)]; c->pc++; break; 
		case 0x66: c->h = c->memory[get_hl(c)]; c->pc++; break; 
		case 0x5e: c->e = c->memory[get_hl(c)]; c->pc++; break;
 		case 0x46: c->b = c->memory[get_hl(c)]; c->pc++; break;
 		case 0x4e: c->c = c->memory[get_hl(c)]; c->pc++; break;

		case 0x77: c->memory[get_hl(c)] = c->a; c->pc++; break;

		// MVI commands
		case 0x3e: c->a = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x06: c->b = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x26: c->h = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x0e: c->c = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x16: c->d = c->memory[c->pc+1]; c->pc+=2; break;
		case 0x36: c->memory[get_hl(c)] = c->memory[c->pc+1]; 			 
			   c->pc+=2;
			   break;

		// ORA commands
		case 0xb6: ora(c->memory[get_hl(c)],c); c->pc++; break;			// ora m	
		case 0xb7: ora(c->a,c); c->pc++; break;					// ora a
		case 0xb1: ora(c->c,c); c->pc++; break;					// ora c
											
		//ADD
		case 0xc6: add(c->memory[c->pc+1], c); c->pc+=2; break;			// adi

		// XRA commands
		case 0xae: xra(c->memory[get_hl(c)],c); c->pc++; break;
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
										
		case 0x12: c->memory[get_de(c)] = c->a; c->pc++; break;			// stax d,e

		case 0x1a: c->a = c->memory[get_de(c)]; c->pc++; break;			// ldax d,e

		case 0x2a:								// lhld 
			c->l = c->memory[make_addr(c)];		
			c->h = c->memory[make_addr(c)+1];	 
			c->pc+=3; break;

		case 0x22: 								// shld
			c->memory[make_addr(c)] = c->l;
			c->memory[make_addr(c)+1] = c->h;
			c->pc+=3; break;

		case 0xf9: c->sp = get_hl(c); c->pc++; break;				// sphl

		// cmp commands
		case 0xfe: cmp(c->memory[c->pc+1], c); c->pc+=2; break;			// cpi
		case 0xbe: cmp(c->memory[get_hl(c)],c); c->pc++; break;			// cmp m

		// RET Addr
		case 0xc9: c->pc = pop_stack(c); break;
		case 0xd8: c->pc = (c->cy == 1) ? pop_stack(c) : c->pc+1; break;
		case 0xd0: c->pc = (c->cy == 0) ? pop_stack(c) : c->pc+1; break;
		case 0xe8: c->pc = (c->pf == 1) ? pop_stack(c) : c->pc+1; break;
		case 0xe0: c->pc = (c->pf == 0) ? pop_stack(c) : c->pc+1; break;
		case 0xc0: c->pc = (c->zf == 0) ? pop_stack(c) : c->pc+1; break;
		case 0xc8: c->pc = (c->zf == 1) ? pop_stack(c) : c->pc+1; break;	// rz
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
		case 0xe9: c->pc = get_hl(c); break;					// pchl

			
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
			  c->b = bc >> 8; c->c = (bc & 0xff);
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
	
		// DCR commands
		case 0x05: c->b = dcr(c->b,c); c->pc++; break;			// dcr b
		case 0x0d: c->c = dcr(c->c,c); c->pc++; break;			// dcr c
					
		// DCX commands
		case 0x2b: dcr_pair(&c->h,&c->l,c); c->pc++; break;		// dcx h
		case 0x0b: dcr_pair(&c->b,&c->c,c); c->pc++; break;		// dcx b
									
		// INR commands
		case 0x3c: c->a = inr(c->a,c); c->pc++; break;			// inr a
		case 0x14: c->d = inr(c->d,c); c->pc++; break;			// inr d
		case 0x34: 							// inr m
			   uint8_t data = c->memory[get_hl(c)];
			   data = inr(data,c); 
			   c->memory[get_hl(c)] = data; 
			   c->pc++; break;
		// INX commands
		case 0x23: inr_pair(&c->h,&c->l,c); c->pc++; break;		// inx h,l 
		case 0x13: inr_pair(&c->d,&c->e,c); c->pc++; break;		// inx d,e
						
		case 0x09: dad(get_bc(c),c); c->pc++; break; 			// dad b,c
		case 0x19: dad(get_de(c),c); c->pc++; break;			// dad d,e
		case 0x29: dad(get_hl(c),c); c->pc++; break;			// dad h,l
		case 0x39: dad(c->sp,c); c->pc++; break;			// dad sp

		case 0xeb:							// xchg h,l with d,e 
			   uint16_t hl_pair = get_hl(c);
			   uint16_t de_pair = get_de(c);
			   uint16_t tmp = hl_pair;
			   hl_pair = de_pair; de_pair = tmp;
			   c->h = hl_pair >> 8; c->l = hl_pair & 0xff;
			   c->d = de_pair >> 8; c->e = de_pair & 0xff;
			   c->pc++; break;

		// rotate
		case 0x0f:							// rrc
			c->cy = c->a & 1;
			c->a >>= 1;
			c->a |= (c->cy << 7);	
			c->pc++;
			break; 
		case 0x07:							// rlc
			c->cy = (c->a >> 7);
			c->a <<= 1;
			c->a |= c->cy;
			c->pc++;
			break;
		
		// interupts
		case 0xf3: c->INTE = 0; c->pc++; break;				// di
		case 0xfb: c->INTE = 1; c->pc++; break;				// ei

		case 0x00: c->pc++; break;					// nop
										//
		case 0x37: c->cy = 1; c->pc++; break;				// stc
										//
		default: 	
			printf("\nUnimplemented: %x\n",opcode);
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
	return 0;
}

void debug(uint8_t opcode,chip* c)
{
	fprintf(stderr,"PC: %04X, AF: %04X, BC: %04X, DE: %04X, HL: %04X, SP: %04X",
			c->pc, c->a << 8 | format_flags(c), get_bc(c), get_de(c), get_hl(c), c->sp);
	fprintf(stderr,"\n");
}

void mini_bdos(chip* c)
{ 
	switch(c->c)
	{
		case 9:
			c->pc = get_de(c);	
			while(c->memory[c->pc] != '$')
			{
				printf("%c",c->memory[c->pc]);	
				c->pc++;
			}
			fflush(stdout);

			// RET to addr after CALL 5
			c->pc = pop_stack(c);
			break;
		case 2:
			printf("%c",c->e);
			break;
		default:
			printf("unimplemented bdos: %d\n",c->c);
			break;
	}
}
