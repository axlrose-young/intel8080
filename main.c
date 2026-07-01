#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "bdos.h"
#include "debugger.h"

bool is_debug = 0;

void chip_init(chip* c){
	FILE* pfile = fopen("cpu_tests/8080EXM.COM","rb"); 

	if(pfile == NULL){ 
		perror("error opening file"); 
	}

	fread(&c->memory[0x100],0xffff,1,pfile);
	fclose(pfile);
	c->pc = 0x100;
	c->is_running = 1;
}

int main(int argc, char** argv){
	chip c;
	chip_init(&c);

	if(argc != 1){
		if(strcmp(argv[1],"-d") == 0){
			is_debug = 1; 
		}	
	}

	while(c.is_running){
		if(is_debug){
			debug(&c);	
		}

		execute(&c);	

		if(c.pc == 5){
			mini_bdos(&c);	
		}
	}

	return 0;
}
