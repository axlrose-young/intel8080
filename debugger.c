#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

int breakpoint = 0;
bool pause = 1;

void debugger_help();
void debugger_message();

int 
debug_loop(chip* c)
{
	printf("d> ");	
			
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
		execute(c);		
		printf("%04X\n",c->pc);
	}
	else if(strcmp(token[0],"q") == 0){
		printf("quitting...\n");
		return 1;	
	}
	else if(strcmp(token[0],"break") == 0){
		breakpoint = strtol(token[1],NULL,16);	
	}			
	else if(strcmp(token[0],"run") == 0){
		printf("running...\n");
		pause = 0;	
	}
	else if (strcmp(token[0],"h") == 0) {
		debugger_help();	
	}
	return 0;
} 

void 
debugger_help()
{
	printf("show intruction and step: s\n");
	printf("to set breakpoint: break <point>\n");
	printf("to run the program: run\n");
	printf("to quit: q\n");
}

void 
debugger_message() 
{
	printf("This is the intel8080 debugger\n");
	printf("Type h for help\n");
}
