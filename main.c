#include <stdio.h>
#include "cpu.h"
#include "bdos.h"

bool is_running = 0;

void chip_init(chip* c){
	FILE* pfile = fopen("cpu_tests/8080EXM.COM","rb"); 

	if(pfile == NULL){ 
		perror("error opening file"); 
	}

	fread(&c->memory[0x100],0xffff,1,pfile);
	fclose(pfile);
	c->pc = 0x100;
	is_running = 1;
}

int main(){
	chip c;
	chip_init(&c);

	while(is_running){
		execute(&c);	

		if(c.pc == 5){
			mini_bdos(&c);	
		}
	}

	return 0;
}
