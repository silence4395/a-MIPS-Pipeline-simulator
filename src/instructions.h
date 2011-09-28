#ifndef __PP_INSTR__
#define __PP_INSTR__

#include <iostream>
#include <fstream>
#include <cstring>
#include "instr_defs.h"
#include "type.h"

using namespace std;

class instruction {
	uint32 instr;
	/*these are the possible parameters*/
	int32 params[5];
	param_type_t ptt[5];

public:
	//For pipeline
	uint32 fetch_cycle;
	uint32 last_processed; //last cycle in which it was processed
	uint32 in_use;


	char str[32];
	/*Fuctions*/
	instruction(){
		instr=0;
		memset(params, 0, sizeof(uint32)*5);
		memset(ptt, 0, sizeof(param_type_t)*5);
		str[0]='\0';
	}
	uint32 decode_instr(uint32 inst, int);
	uint32 execute_instr();
	uint32 disassemble_instr(uint32 inst, int);
	uint32 memory_access();
	uint32 is_fetch_feasible();
	void get_src_dest(int *dest, int *src);
	void read_registers(int );
	uint32 execute_branch();
	uint32 write_back();
	uint32 is_mem_instr();
};

int check_hazards(instruction *);
uint32 fetch_decode(instruction *);
uint32 issue_stage(instruction *, instruction *);
uint32 execute_stage(instruction *, instruction *);	
uint32 memory_stage(instruction *, instruction *);
uint32 writeback_stage(instruction *, instruction *);

#endif
