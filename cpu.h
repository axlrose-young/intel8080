#ifndef CPU_H
#define CPU_H
#include <stdio.h>
#include <stdint.h>

typedef struct {
	uint8_t memory[0xffff];
	uint16_t pc;

	// work registers
	uint8_t a,b,c,d,e,h,l; 

	//flags
	bool sf, zf, pf, cy, ac;

	uint16_t sp;
	
	bool INTE, is_running;
}chip;

void execute(chip* c);

#endif
