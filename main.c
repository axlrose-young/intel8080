#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpu.h"
#include "bdos.h"
//#include "debugger.h"
int breakpoint = 0;
bool pause = 1;
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
			if(pause){
				printf("d>");	
				
				char* buffer = malloc(50);
				fgets(buffer,50,stdin);

				buffer[strlen(buffer) - 1] = '\0';	// removes \n	

				// tokenizing the buffer
				char* token[5];				
				int index = 0;

				token[0] = strtok(buffer, " ");
				while(token[index] != NULL){
					index++;
					token[index] = strtok(NULL, " ");
				}
				token[index] = NULL;

				if(strcmp(token[0],"s") == 0){
					execute(&c);		
					printf("%04X\n",c.pc);
				}
				else if(strcmp(token[0],"q") == 0){
					printf("quitting...\n");
					break;	
				}
				else if(strcmp(token[0],"break") == 0){
					breakpoint = strtol(token[1],NULL,16);	
				}			
				else if(strcmp(token[0],"run") == 0){
					printf("running...\n");
					pause = 0;	
				}
				
			}

			if(c.pc == breakpoint){
				printf("PC: %04X\n",c.pc);
				pause = 1;
				continue;
			}
			execute(&c);
		}
		else{
			execute(&c);
			if(c.pc == 5) { mini_bdos(&c); }
		}
	}

	return 0;
}
