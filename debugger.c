#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger.h"

void debug(chip* c){
	printf("d>");	
	c->is_running = 0;
	
	char* buffer = malloc(50);
	fgets(buffer,50,stdin);

	buffer[strlen(buffer) - 1] = '\0';	
	printf("%s\n",buffer);
} 
