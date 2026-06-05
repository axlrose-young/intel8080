#include <stdio.h>
#include <stdint.h>
#include <string.h>

// flag to stop main emulator loop
int is_running = 0;

// flag to check if debug
int debug_mode = 1;

typedef struct {
	uint8_t memory[0xFFFF]; 
	uint16_t pc;
}chip;	

typedef struct {
	char* name;
	uint8_t size;
	int (*execute)(chip*, uint8_t);
}instructions;

// All instr prototypes
int exec_c3(chip* mychip, uint8_t opcode);

instructions table[256] = {
    [0x00] = {"NOP", 1},
    [0x01] = {"LXI B,D16", 3},
    [0x02] = {"STAX B", 1},
    [0x03] = {"INX B", 1},
    [0x04] = {"INR B", 1},
    [0x05] = {"DCR B", 1},
    [0x06] = {"MVI B,D8", 2},
    [0x07] = {"RLC", 1},
    [0x08] = {"---", 0},
    [0x09] = {"DAD B", 1},
    [0x0A] = {"LDAX B", 1},
    [0x0B] = {"DCX B", 1},
    [0x0C] = {"INR C", 1},
    [0x0D] = {"DCR C", 1},
    [0x0E] = {"MVI C,D8", 2},
    [0x0F] = {"RRC", 1},
    [0x10] = {"---", 0},
    [0x11] = {"LXI D,D16", 3},
    [0x12] = {"STAX D", 1},
    [0x13] = {"INX D", 1},
    [0x14] = {"INR D", 1},
    [0x15] = {"DCR D", 1},
    [0x16] = {"MVI D,D8", 2},
    [0x17] = {"RAL", 1},
    [0x18] = {"---", 0},
    [0x19] = {"DAD D", 1},
    [0x1A] = {"LDAX D", 1},
    [0x1B] = {"DCX D", 1},
    [0x1C] = {"INR E", 1},
    [0x1D] = {"DCR E", 1},
    [0x1E] = {"MVI E,D8", 2},
    [0x1F] = {"RAR", 1},
    [0x20] = {"---", 0},
    [0x21] = {"LXI H,D16", 3},
    [0x22] = {"SHLD Adr", 3},
    [0x23] = {"INX H", 1},
    [0x24] = {"INR H", 1},
    [0x25] = {"DCR H", 1},
    [0x26] = {"MVI H,D8", 2},
    [0x27] = {"DAA", 1},
    [0x28] = {"---", 0},
    [0x29] = {"DAD H", 1},
    [0x2A] = {"LHLD Adr", 3},
    [0x2B] = {"DCX H", 1},
    [0x2C] = {"INR L", 1},
    [0x2D] = {"DCR L", 1},
    [0x2E] = {"MVI L,D8", 2},
    [0x2F] = {"CMA", 1},
    [0x30] = {"SIM", 1},
    [0x31] = {"LXI SP,D16", 3},
    [0x32] = {"STA Adr", 3},
    [0x33] = {"INX SP", 1},
    [0x34] = {"INR M", 1},
    [0x35] = {"DCR M", 1},
    [0x36] = {"MVI M,D8", 2},
    [0x37] = {"STC", 1},
    [0x38] = {"---", 0},
    [0x39] = {"DAD SP", 1},
    [0x3A] = {"LDA Adr", 3},
    [0x3B] = {"DCX SP", 1},
    [0x3C] = {"INR A", 1},
    [0x3D] = {"DCR A", 1},
    [0x3E] = {"MVI A,D8", 2},
    [0x3F] = {"CMC", 1},
    [0x40] = {"MOV B,B", 1},
    [0x41] = {"MOV B,C", 1},
    [0x42] = {"MOV B,D", 1},
    [0x43] = {"MOV B,E", 1},
    [0x44] = {"MOV B,H", 1},
    [0x45] = {"MOV B,L", 1},
    [0x46] = {"MOV B,M", 1},
    [0x47] = {"MOV B,A", 1},
    [0x48] = {"MOV C,B", 1},
    [0x49] = {"MOV C,C", 1},
    [0x4A] = {"MOV C,D", 1},
    [0x4B] = {"MOV C,E", 1},
    [0x4C] = {"MOV C,H", 1},
    [0x4D] = {"MOV C,L", 1},
    [0x4E] = {"MOV C,M", 1},
    [0x4F] = {"MOV C,A", 1},
    [0x50] = {"MOV D,B", 1},
    [0x51] = {"MOV D,C", 1},
    [0x52] = {"MOV D,D", 1},
    [0x53] = {"MOV D,E", 1},
    [0x54] = {"MOV D,H", 1},
    [0x55] = {"MOV D,L", 1},
    [0x56] = {"MOV D,M", 1},
    [0x57] = {"MOV D,A", 1},
    [0x58] = {"MOV E,B", 1},
    [0x59] = {"MOV E,C", 1},
    [0x5A] = {"MOV E,D", 1},
    [0x5B] = {"MOV E,E", 1},
    [0x5C] = {"MOV E,H", 1},
    [0x5D] = {"MOV E,L", 1},
    [0x5E] = {"MOV E,M", 1},
    [0x5F] = {"MOV E,A", 1},
    [0x60] = {"MOV H,B", 1},
    [0x61] = {"MOV H,C", 1},
    [0x62] = {"MOV H,D", 1},
    [0x63] = {"MOV H,E", 1},
    [0x64] = {"MOV H,H", 1},
    [0x65] = {"MOV H,L", 1},
    [0x66] = {"MOV H,M", 1},
    [0x67] = {"MOV H,A", 1},
    [0x68] = {"MOV L,B", 1},
    [0x69] = {"MOV L,C", 1},
    [0x6A] = {"MOV L,D", 1},
    [0x6B] = {"MOV L,E", 1},
    [0x6C] = {"MOV L,H", 1},
    [0x6D] = {"MOV L,L", 1},
    [0x6E] = {"MOV L,M", 1},
    [0x6F] = {"MOV L,A", 1},
    [0x70] = {"MOV M,B", 1},
    [0x71] = {"MOV M,C", 1},
    [0x72] = {"MOV M,D", 1},
    [0x73] = {"MOV M,E", 1},
    [0x74] = {"MOV M,H", 1},
    [0x75] = {"MOV M,L", 1},
    [0x76] = {"HLT", 1},
    [0x77] = {"MOV M,A", 1},
    [0x78] = {"MOV A,B", 1},
    [0x79] = {"MOV A,C", 1},
    [0x7A] = {"MOV A,D", 1},
    [0x7B] = {"MOV A,E", 1},
    [0x7C] = {"MOV A,H", 1},
    [0x7D] = {"MOV A,L", 1},
    [0x7E] = {"MOV A,M", 1},
    [0x7F] = {"MOV A,A", 1},
    [0x80] = {"ADD B", 1},
    [0x81] = {"ADD C", 1},
    [0x82] = {"ADD D", 1},
    [0x83] = {"ADD E", 1},
    [0x84] = {"ADD H", 1},
    [0x85] = {"ADD L", 1},
    [0x86] = {"ADD M", 1},
    [0x87] = {"ADD A", 1},
    [0x88] = {"ADC B", 1},
    [0x89] = {"ADC C", 1},
    [0x8A] = {"ADC D", 1},
    [0x8B] = {"ADC E", 1},
    [0x8C] = {"ADC H", 1},
    [0x8D] = {"ADC L", 1},
    [0x8E] = {"ADC M", 1},
    [0x8F] = {"ADC A", 1},
    [0x90] = {"SUB B", 1},
    [0x91] = {"SUB C", 1},
    [0x92] = {"SUB D", 1},
    [0x93] = {"SUB E", 1},
    [0x94] = {"SUB H", 1},
    [0x95] = {"SUB L", 1},
    [0x96] = {"SUB M", 1},
    [0x97] = {"SUB A", 1},
    [0x98] = {"SBB B", 1},
    [0x99] = {"SBB C", 1},
    [0x9A] = {"SBB D", 1},
    [0x9B] = {"SBB E", 1},
    [0x9C] = {"SBB H", 1},
    [0x9D] = {"SBB L", 1},
    [0x9E] = {"SBB M", 1},
    [0x9F] = {"SBB A", 1},
    [0xA0] = {"ANA B", 1},
    [0xA1] = {"ANA C", 1},
    [0xA2] = {"ANA D", 1},
    [0xA3] = {"ANA E", 1},
    [0xA4] = {"ANA H", 1},
    [0xA5] = {"ANA L", 1},
    [0xA6] = {"ANA M", 1},
    [0xA7] = {"ANA A", 1},
    [0xA8] = {"XRA B", 1},
    [0xA9] = {"XRA C", 1},
    [0xAA] = {"XRA D", 1},
    [0xAB] = {"XRA E", 1},
    [0xAC] = {"XRA H", 1},
    [0xAD] = {"XRA L", 1},
    [0xAE] = {"XRA M", 1},
    [0xAF] = {"XRA A", 1},
    [0xB0] = {"ORA B", 1},
    [0xB1] = {"ORA C", 1},
    [0xB2] = {"ORA D", 1},
    [0xB3] = {"ORA E", 1},
    [0xB4] = {"ORA H", 1},
    [0xB5] = {"ORA L", 1},
    [0xB6] = {"ORA M", 1},
    [0xB7] = {"ORA A", 1},
    [0xB8] = {"CMP B", 1},
    [0xB9] = {"CMP C", 1},
    [0xBA] = {"CMP D", 1},
    [0xBB] = {"CMP E", 1},
    [0xBC] = {"CMP H", 1},
    [0xBD] = {"CMP L", 1},
    [0xBE] = {"CMP M", 1},
    [0xBF] = {"CMP A", 1},
    [0xC0] = {"RNZ", 1},
    [0xC1] = {"POP B", 1},
    [0xC2] = {"JNZ Adr", 3},
    [0xC3] = {"JMP Adr", 3, exec_c3},
    [0xC4] = {"CNZ Adr", 3},
    [0xC5] = {"PUSH B", 1},
    [0xC6] = {"ADI D8", 2},
    [0xC7] = {"RST 0", 1},
    [0xC8] = {"RZ", 1},
    [0xC9] = {"RET", 1},
    [0xCA] = {"JZ Adr", 3},
    [0xCB] = {"---", 0},
    [0xCC] = {"CZ Adr", 3},
    [0xCD] = {"CALL Adr", 3},
    [0xCE] = {"ACI D8", 2},
    [0xCF] = {"RST 1", 1},
    [0xD0] = {"RNC", 1},
    [0xD1] = {"POP D", 1},
    [0xD2] = {"JNC Adr", 3},
    [0xD3] = {"OUT D8", 2},
    [0xD4] = {"CNC Adr", 3},
    [0xD5] = {"PUSH D", 1},
    [0xD6] = {"SUI D8", 2},
    [0xD7] = {"RST 2", 1},
    [0xD8] = {"RC", 1},
    [0xD9] = {"---", 0},
    [0xDA] = {"JC Adr", 3},
    [0xDB] = {"IN D8", 2},
    [0xDC] = {"CC Adr", 3},
    [0xDD] = {"---", 0},
    [0xDE] = {"SBI D8", 2},
    [0xDF] = {"RST 3", 1},
    [0xE0] = {"RPO", 1},
    [0xE1] = {"POP H", 1},
    [0xE2] = {"JPO Adr", 3},
    [0xE3] = {"XTHL", 1},
    [0xE4] = {"CPO Adr", 3},
    [0xE5] = {"PUSH H", 1},
    [0xE6] = {"ANI D8", 2},
    [0xE7] = {"RST 4", 1},
    [0xE8] = {"RPE", 1},
    [0xE9] = {"PCHL", 1},
    [0xEA] = {"JPE Adr", 3},
    [0xEB] = {"XCHG", 1},
    [0xEC] = {"CPE Adr", 3},
    [0xED] = {"---", 0},
    [0xEE] = {"XRI D8", 2},
    [0xEF] = {"RST 5", 1},
    [0xF0] = {"RP", 1},
    [0xF1] = {"POP PSW", 1},
    [0xF2] = {"JP Adr", 3},
    [0xF3] = {"DI", 1},
    [0xF4] = {"CP Adr", 3},
    [0xF5] = {"PUSH PSW", 1},
    [0xF6] = {"ORI D8", 2},
    [0xF7] = {"RST 6", 1},
    [0xF8] = {"RM", 1},
    [0xF9] = {"SPHL", 1},
    [0xFA] = {"JM Adr", 3},
    [0xFB] = {"EI", 1},
    [0xFC] = {"CM Adr", 3},
    [0xFD] = {"---", 0},
    [0xFE] = {"CPI D8", 2},
    [0xFF] = {"RST 7", 1}
}; 

void init_load_rom(chip* mychip)
{
	FILE* pfile = fopen("8080.COM","rb");
	if(!pfile)
	{
		perror("Error loading rom");
	}

	fread(mychip->memory, 1,0xFFFF , pfile);
}

void disassemble(uint8_t opcode, chip* mychip)
{
	// print pc, opcode, instr name, no.of bytes	
	printf("pc: %d   opcode: %x   name: %s   bytes: %d\n",mychip->pc, opcode, 						table[opcode].name, table[opcode].size);
}

int fetch_execute(chip* mychip, int debug_mode)
{
	if(mychip->pc >= 0xFFFF)
	{
		is_running = 0;	
	}
	// getting current opcode
	uint8_t opcode = mychip->memory[mychip->pc];	
	// getting instruction length
	uint8_t instr = table[opcode].size;

	// disassembler print
	if(debug_mode == 1)
	{
		disassemble(opcode, mychip);
	}

	// increment pc as per instruction (variable)
	// some instr size is 0 hence increment by 1
	if(instr == 0)
	{
		mychip->pc+=1;	
	}
	else
	{
		mychip->pc+=instr;
	}

	// run the executor
	return table[opcode].execute(mychip, opcode);
}

int main()
{
	// init struct
	chip mychip = {0};

	// loading rom into memory
	init_load_rom(&mychip);	

	// fetch codes
	is_running = 1;
	while(is_running)
	{
		int cycle_count = 0;
		cycle_count += fetch_execute(&mychip, debug_mode);	
	}
	return 0;
}

int exec_c3(chip* mychip, uint8_t opcode)
{
	uint8_t high_addr = mychip->pc+=2;	
	uint8_t low_addr = mychip->pc+=1;	

	uint16_t addr = high_addr << 8 | low_addr;
	mychip->pc = addr;
	
	return 10;
}
