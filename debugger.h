#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "cpu.h"

extern int breakpoint;
extern bool pause;

int debug_loop(chip* c);

#endif


