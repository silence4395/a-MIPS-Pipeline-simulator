/* -*- linux-c -*-
 * instructions.cc
 * Provides API's to push an instruction through DECODE and EXECUTE stages.
 */
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

#include "instructions.h"
#include "type.h"
#include "memory.h"
#include "utility.h"

extern memory reg;
extern memory ram;
extern uint32 max_address;

/*
 *These variable are updated by fetch_decode(),
 *To indicate the status of status of fetch_decode block.
 */
char exec_inst[32];
char wait_inst[32];


/*
 * Returns a number formed by bits b/w m and n (including m and n)
 * m should be greater than n (m > n)
 * and 0 <= n < m <= 31
 */

inline uint32 extract_data(uint32 num,int m, int n)
{
	num = num << ((INSTRUCTION_WIDTH-1)-m);
	num = num >> ((INSTRUCTION_WIDTH-1)-m+n);
	return num;
}

int fetch_reg_map[32];
int issue_reg_map[32];

#define MARK_REG_USAGE_FETCH(A) (fetch_reg_map[(A)]++)
#define MARK_REG_USAGE_ISSUE(A) (issue_reg_map[(A)]++)

#define UNMARK_REG_USAGE_FETCH(A) (fetch_reg_map[(A)]--)
#define UNMARK_REG_USAGE_ISSUE(A) (issue_reg_map[(A)]--)

#define CHECK_REG_USAGE_FETCH(A) (fetch_reg_map[(A)])
#define CHECK_REG_USAGE_ISSUE(A) (issue_reg_map[(A)])

/*
 ****** is_fetch_possible() *****
 *Returns:
 *      0 -- If the instruction is branch and can't be fetched
 *      1 -- If the instruction is branch and can be fetched
 *      2 -- If the instruction is not a branch
 */
uint32 instruction:: is_fetch_feasible() 
{
        switch (params[0]) {
	case OP_SPECIAL:
                if (params[1] == OP_SPECIAL_JR)  {
                        /*if register not used*/
                        if (CHECK_REG_USAGE_FETCH(ptt[2]) == 0)
                                return 1;
                        else
                                return 0;
                }
                else if (params[1] == OP_SPECIAL_BREAK)
                        return 1;
                else if (instr == 0) /*NOP*/
                        return 1;
                else
                        return 2; //sll, sra, srl
                break;

        case OP_J:
                return 1;
                break;

        case OP_BEQ:
        case OP_BNE:
                if (CHECK_REG_USAGE_FETCH(ptt[1]) == 0 &&
                    CHECK_REG_USAGE_FETCH(ptt[2]) == 0)
                        return 1;
                else
                        return 0;
                break;

        case OP_BGTZ:
        case OP_BLEZ: 
                if (CHECK_REG_USAGE_FETCH(ptt[1]) == 0)
                        return 1;
                else
                        return 0;
                break;

        case OP_REGIMM:
                if (params[1] == OP_REGIMEM_BGEZ || params[1] == OP_REGIMEM_BLTZ) {
                        if (CHECK_REG_USAGE_FETCH(ptt[2]) == 0)
                                return 1;
                        else
                                return 0;
                }
		else //Not a branch
			return 2;
                break;

        default:
                return 2;
                break;
        }
	fprintf (stderr, "%d [%s] Control should never come here!!\n",__LINE__, str);
	return 2;
}

/*
 ******* disassemble_instr()********
 *
 *Returns 1 when the decoded instruction is BREAK
 *else returns 0
 *
 *Pamameters of an instruction are read into params[] and ptt[]
 */
uint32 instruction::disassemble_instr(uint32 inst , int update_register_map)
{
	uint32 ret=0;
	int32 temp;
	char tmp[16];

	//Get the primary Opcode
	instr = inst;
	params[0] = instr >> (INSTRUCTION_WIDTH - OPCODE_WIDTH);
	ptt[0] = OPCODE;

	switch (params[0]) {
		//this is the opcode for J, BREAK,SLL,SRL,SRA,NOP
	case OP_SPECIAL:
		temp = extract_data(inst,5,0);
		params[1] = temp;
		ptt[1]=OPCODE;

		switch(temp) {
			//jr rs
		case OP_SPECIAL_JR:
			temp = extract_data(inst,25,21);
			params[2] = reg.fetch_data(RADDR(temp));
			ptt[2] = (param_type_t)temp;
			sprintf(str,"JR\tR%d",temp);
			break;

		case OP_SPECIAL_BREAK:
			ret=1; //indicate break to caller
			sprintf(str,"BREAK") ;
			break;
			//SRL rd, rt, sa
			//SLL rd, rt, sa
			//SRA rd, rs, sa
			//NOP
		case OP_SPECIAL_SLL:
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
			//case OP_SPECIAL_NOP:
			temp = extract_data(inst,20,16); //rt
			params[2] = reg.fetch_data(RADDR(temp));
			ptt[2] = (param_type_t)temp;

			temp = extract_data(inst,15,11); //rd
			params[3] = reg.fetch_data(RADDR(temp));
			ptt[3] = (param_type_t)temp;

                        if (update_register_map)
                                MARK_REG_USAGE_FETCH(ptt[3]);

			temp = extract_data(inst,10,6); //sa
			params[4] = temp;
			ptt[4] = IMMEDIATE5;

			if (params[1] == OP_SPECIAL_SRL)
				sprintf (str,"SRL\tR%d, R%d, #%d",ptt[3],ptt[2],params[4]);
			else if (params[1] == OP_SPECIAL_SRA)
				sprintf (str, "SRA\tR%d, R%d, #%d",ptt[3],ptt[2],params[4]);
			else if (inst == 0)
				sprintf(str,"NOP");
			else
				sprintf(str,"SLL\tR%d, R%d, #%d",ptt[3],ptt[2],params[4]);
			break;
		}
		break;
		//covers OP_BGEZ and OP_BLTZ
	case OP_REGIMM :
		temp = extract_data(inst,20,16);
		params[1] = temp;
		ptt[1]=OPCODE;

		temp = extract_data(inst,25,21); //rs
		params[2] = reg.fetch_data(RADDR(temp));
		ptt[2] = (param_type_t)temp;

		temp = extract_data(inst,15,0);
                params[3] = extend_sign_bit32(temp, 15);
		params[3] = params[3]<<2;
		ptt[3] = IMMEDIATE16;

		if (params[1]== OP_REGIMEM_BGEZ)
			sprintf(str, "BGEZ\tR%d, #%d",ptt[2],params[3]);
		else if (params[1] == OP_REGIMEM_BLTZ)
			sprintf(str, "BLTZ\tR%d, #%d",ptt[2],params[3]);
		else
			cout << "undefined exception\n";
		break;

		//J <Immediate Address>
	case OP_J:
		params[1] = extract_data(inst,25,0);
		params[1] <<= 2;
		ptt[1] = ADDRESS28;
		sprintf(str,"J\t#%d",params[1]);
		break;

		//BEQ rs, rt, offset
		//BNE rs, rt, offset
	case OP_BEQ:
	case OP_BNE:
		temp = extract_data(inst,25,21); //rs
		params[1] = reg.fetch_data(RADDR(temp));
		ptt[1] = (param_type_t)temp;

		temp = extract_data(inst,20,16); //rt
		params[2] = reg.fetch_data(RADDR(temp));
		ptt[2] = (param_type_t)temp;

		temp = extract_data(inst,15,0); //offset
                params[3] = extend_sign_bit32(temp,15);
		params[3] = params[3]<<2;
		ptt[3] = ADDRESS18;

		if (params[0] == OP_BEQ)
			sprintf(str,"BEQ\tR%d, R%d, #%d",ptt[1],ptt[2],params[3]);
		else
			sprintf(str,"BNE\tR%d, R%d, #%d",ptt[1],ptt[2],params[3]);

		break;

		//BGTZ rs, offset
		//BLEZ rs, offset
	case OP_BGTZ :
	case OP_BLEZ :
		temp = extract_data(inst,25,21);
		params[1] = reg.fetch_data(RADDR(temp));
		ptt[1] = (param_type_t)temp;

		temp = extract_data(inst,15,0);
                temp = extend_sign_bit32(temp, 15);
		params[2] = temp<<2;
		ptt[2] = ADDRESS18;

		if (params[0] == OP_BGTZ)
			sprintf(str,"BGTZ\tR%d, #%d",ptt[1],params[2]);
		else
		        sprintf(str,"BLEZ\tR%d, #%d",ptt[1],params[2]);
		break;

		//SW rt, offset(base)
	case OP_SW :
	case OP_LW :
		temp = extract_data (inst, 20,16); //rt
		params[1] = reg.fetch_data(RADDR(temp));
		ptt[1]=(param_type_t)temp;

                if (update_register_map && params[0]==OP_LW)
                        MARK_REG_USAGE_FETCH(ptt[1]);

		temp = extract_data (inst, 15,0); //offset
		params[2] = temp;
                params[2] = extend_sign_bit32(params[2],15);
		ptt[2]=IMMEDIATE16;

		temp = extract_data (inst, 25,21); //base
		params[3] = reg.fetch_data(RADDR(temp));
		ptt[3]=(param_type_t)temp;

		if (params[0] == OP_SW)
			sprintf(str,"SW\tR%d, %d(R%d)",ptt[1],params[2],ptt[3]);
		else
			sprintf(str,"LW\tR%d, %d(R%d)",ptt[1],params[2],ptt[3]);
		break;

		//<inst> rd rs rt
	case OP_ADD  :
		sprintf(tmp,"ADD\tR"); goto COMMON_CAT2;
	case OP_SUB  :
		sprintf(tmp,"SUB\tR"); goto COMMON_CAT2;
	case OP_MUL  :
		sprintf(tmp,"MUL\tR"); goto COMMON_CAT2;
	case OP_AND  :
		sprintf(tmp,"AND\tR"); goto COMMON_CAT2;
	case OP_OR   :
		sprintf(tmp,"OR\tR"); goto COMMON_CAT2;
	case OP_XOR  :
		sprintf(tmp,"XOR\tR"); goto COMMON_CAT2;
	case OP_NOR  :
		sprintf(tmp,"NOR\tR"); goto COMMON_CAT2;
	case OP_SLT  :
		sprintf(tmp,"SLT\tR"); goto COMMON_CAT2;
	COMMON_CAT2:
		temp = extract_data (inst, 25,21); //rs
		params[1] = reg.fetch_data(RADDR(temp));
		ptt[1]=(param_type_t)temp;

		temp = extract_data (inst, 20,16); //rt
		params[2] = reg.fetch_data(RADDR(temp));
		ptt[2]=(param_type_t)temp;

		temp = extract_data (inst, 15,11); //rd
		params[3] = temp;
		ptt[3]=(param_type_t)temp;

                if (update_register_map)
                        MARK_REG_USAGE_FETCH(ptt[3]);

		sprintf(str,"%s%d, R%d, R%d",tmp,ptt[3],ptt[1],ptt[2]);
		break;

		//ADD rt rs IMMEDIATE
	case OP_ADDI :
		sprintf(tmp,"ADDI\tR"); goto COMMON_CAT3;
	case OP_ANDI :
		sprintf(tmp,"ANDI\tR"); goto COMMON_CAT3;
	case OP_ORI  :
		sprintf(tmp,"ORI\tR"); goto COMMON_CAT3;
	case OP_XORI :
		sprintf(tmp,"XORI\tR"); goto COMMON_CAT3;
	case OP_MOVZ :
		sprintf(tmp,"MOVZ\tR"); goto COMMON_CAT3;
	COMMON_CAT3:
		temp = extract_data (inst, 20,16); //rt
		params[1] = reg.fetch_data(RADDR(temp));
		ptt[1]=(param_type_t)temp;

                if (update_register_map && params[0] != OP_MOVZ)
                        MARK_REG_USAGE_FETCH(ptt[1]);
                else
                        MARK_REG_USAGE_FETCH(ptt[2]);

	      	temp = extract_data (inst, 25,21); //rs
		params[2] = reg.fetch_data(RADDR(temp));
		ptt[2]=(param_type_t)temp;

		temp = extract_data (inst, 15,0); //immediate
                if(params[0]==OP_ADDI || params[0] == OP_MOVZ)
                        temp = extend_sign_bit32(temp,15);
		params[3] = temp;
		ptt[3]=IMMEDIATE16;
                if (params[0] != OP_MOVZ)
                        sprintf(str,"%s%d, R%d, #%d",tmp,ptt[1], ptt[2],params[3]);
                else
                        sprintf(str,"%s%d, R%d, #%d",tmp,ptt[2], ptt[1],params[3]);
		break;
	}
	return ret;
}

/*
 * just a dummy with a different name same as disassemble
 */
uint32 instruction::decode_instr(uint32 inst, int update_register_map)
{
	return disassemble_instr(inst, update_register_map);
}


/*
 * Returns 1 if the executed instruction was a break else zero
 */
/*
 *Implements the top level routine that implements the fetch-decode stage
 */
uint32 fetch_decode (instruction *pre_issue)
{
        uint32 inst,pc;
        int32 i;
        uint32 ret=0;

        //initialize exec_inst and wait_inst
        exec_inst[0]=wait_inst[0]='\0';

        /*Check if we have a free slot*/
        for (i=0 ; i<PRE_ISSUE_BUFFER_SIZE ; i++) {
                if (pre_issue[i].in_use == false)
                        break;
        }

        if (i >= PRE_ISSUE_BUFFER_SIZE)/*No free slots*/
                return ret;

        //read PC
        pc = reg.fetch_data(PC_ADDR);
        //read instruction
        inst = ram.fetch_data(pc);

        //decode the instruction, without updating register usage map
        pre_issue[i].decode_instr(inst,0);

        //branch - fetch possible
        if (pre_issue[i].is_fetch_feasible() == 1) {
                //pc = pc + 4
                reg.store_data(PC_ADDR, pc+4);

                ret = pre_issue[i].execute_branch();
                if (ret)
                        reg.store_data(PC_ADDR, pc-4);

                strcpy (exec_inst, pre_issue[i].str);
        } 
        else if (pre_issue[i].is_fetch_feasible() == 2) {// not a branch
                //pc = pc + 4
                reg.store_data(PC_ADDR, pc+4);
                //Update register map
                pre_issue[i].decode_instr(inst,1);
                //this makes the decoded instruction valid
                pre_issue[i].in_use = true;
                pre_issue[i].last_processed = get_cycle();
                pre_issue[i].fetch_cycle = get_cycle();
        }
        else {
                strcpy(wait_inst, pre_issue[i].str);
        }
        if (ret)
                fprintf (stderr, "BREAK\n\n");
        return ret;

}


/*
 * Arguments -- pointers pre-issue buffer and pre-alu buffer
 *Implements the Issue stage of Project
 */
uint32 issue_stage (instruction *pre_issue, instruction *pre_alu)
{
        int32 pre_alu_off;
        int32 pre_issue_off;

        /*check for empty slots in pre-alu*/
        for (pre_alu_off = 0; pre_alu_off < PRE_ALU_BUFFER_SIZE; pre_alu_off++) {
                if (pre_alu[pre_alu_off].in_use==false)
                        break;
        }
        /*structural hazard*/
        if (pre_alu_off >= PRE_ALU_BUFFER_SIZE)
                return 1;

        /*check for any avilable data to process*/        
        for (pre_issue_off = 0; pre_issue_off < PRE_ISSUE_BUFFER_SIZE ; pre_issue_off++){
                if ((pre_issue[pre_issue_off].in_use == true) && //is it valid
                    pre_issue[pre_issue_off].last_processed < get_cycle()) // was it added in previous simulated cpu cycle
                        break;
        }

        /*no valid data for processing*/
        if (pre_issue_off >= PRE_ISSUE_BUFFER_SIZE)
                return 1;

        pre_issue_off = check_hazards(pre_issue);

        if (pre_issue_off == -1)
                return 1;
        
        //read register and upadte register usage map
        pre_issue[pre_issue_off].read_registers(1);

        //copy from pre_issue to pre_alu buffer
        pre_alu[pre_alu_off] = pre_issue[pre_issue_off];
        pre_alu[pre_alu_off].last_processed = get_cycle();

	//invalidate the pre_issue entry
        pre_issue[pre_issue_off].in_use = false;

        //move all the entries to the lower indices
	while (pre_issue_off != PRE_ISSUE_BUFFER_SIZE-1) {
                pre_issue[pre_issue_off]=pre_issue[pre_issue_off+1];
                pre_issue[pre_issue_off+1].in_use = false;
		pre_issue_off++;
        }
	return 0;
}


/*
 * Arguments --- pointers to pre_alu and post_alu buffers
 *ALU of execution stage
 */
uint32 execute_stage (instruction *pre_alu, instruction *post_alu)
{
        static int last_inst=-1;
        static int wait=0;

        if (wait) {
                wait = 0;
                post_alu[0] = pre_alu[last_inst];
                pre_alu[last_inst].in_use = false;
                return 0;
        }

        for (last_inst=0 ; last_inst < PRE_ALU_BUFFER_SIZE ; last_inst++) {
                if (pre_alu[last_inst].in_use == true)
                        break;
        }

        if (last_inst >= PRE_ALU_BUFFER_SIZE)
                return 1;

        wait = pre_alu[last_inst].execute_instr();
        if (!wait) {
                //since we no need to wait -- invalidate
                post_alu[0] = pre_alu[last_inst];
                pre_alu[last_inst].in_use = false;
        }
	return 0;
}

uint32 memory_stage (instruction *post_alu, instruction *post_mem)
{
        int ret;
	if (post_alu[0].in_use == false)
		return 0;

        ret = post_alu[0].memory_access();
        if(ret == OP_LW) {
                post_mem[0] = post_alu[0];
                post_alu[0].in_use = false;
                post_mem[0].in_use = true;
        } 
        else {
                post_alu[0].in_use = false;
        }
	return 0;
}


uint32 instruction::is_mem_instr()
{
        if (params[0]==OP_LW || params[0]==OP_SW)
                return params[0];
        else 
                return 0;
}

uint32 writeback_stage (instruction *post_alu, instruction *post_mem)
{
	if (post_alu[0].in_use == false  && post_mem[0].in_use == false) {
		return 0;
	}

        if (!post_alu[0].is_mem_instr()) {
                post_alu[0].write_back();
                post_alu[0].in_use=false;
        }

        if (post_mem[0].in_use == true) {
                post_mem[0].write_back();
                post_mem[0].in_use=false;
        }

	return 0;
}


/*
 *  execute_instr()
 *
 *Execute stage of the instruction
 *It is assumed PC=PC+<word_length> is done at fetch
 *
 *data for execution stage like computed address is stored into, the first not-useful(unused or not useful at exec) slot in params[]
 */
uint32 instruction::execute_instr()
{
	uint32 ret=0;
	int32 t_reg1, t_reg2,i;

	switch (params[0]) {
	case OP_SPECIAL:
		switch (params[1]) {

		case OP_SPECIAL_SLL: 
			if (instr) { //SLL
				t_reg1 = params[2]; //reg.fetch_dta(RADDR(ptt[2]));
				t_reg1 = t_reg1 << params[4];
				params[3] = t_reg1;   //reg.store_data (RADDR(ptt[3]), t_reg1);
                                ret=1;
			}
			break;
		case OP_SPECIAL_SRL:
			t_reg1 = params[2];      //reg.fetch_data(RADDR(ptt[2]));
			t_reg1 = (t_reg1 >> params[4]);
			t_reg2 = 0x7FFFFFFF;
                        for (i=0 ; i<params[4] ;i++) {
                                t_reg1 = (t_reg1) & (t_reg2);
                                t_reg2 = t_reg2 >> 1;
                        }
			params[3] = t_reg1;      //reg.store_data (RADDR(ptt[3]), t_reg1);
                        ret =1;
			break;

		case OP_SPECIAL_SRA:
			t_reg1 = params[2];       //reg.fetch_data(RADDR(ptt[2]));
			t_reg2 = t_reg1 & 0x80000000;
			if (t_reg2)
				for (i=0 ; i<params[4] ;i++)
					t_reg2 = (t_reg2>>1) | 0x80000000;
			t_reg1 = (t_reg1 >> params[4]) | t_reg2;
			params[3] = t_reg1;//reg.store_data (RADDR(ptt[3]), t_reg1);
                        ret = 1;
			break;
		}
		break;

	case OP_SW :
		t_reg1 = params[3];//reg.fetch_data(RADDR(ptt[3]));
		t_reg1 = t_reg1 + params[2];
		params[2] = t_reg1; // will be read in memory access stage
		break;

	case OP_LW :
		t_reg1 = params[3];//reg.fetch_data(RADDR(ptt[3]));
		t_reg1 = t_reg1 + params[2];
		params[2] = t_reg1; // will be read in memory access stage
		break;

	case OP_ADD :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3] = t_reg1 + t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 + t_reg2);
		break;

	case OP_SUB :	
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3]= t_reg1 - t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 - t_reg2);	
		break;

	case OP_MUL :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3] = t_reg1 * t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 * t_reg2);
                ret = 1;
		break;

	case OP_AND :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3] = t_reg1 & t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 & t_reg2);
		break;

	case OP_OR  :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3]=t_reg1 | t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 | t_reg2);
		break;

	case OP_XOR :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3]=t_reg1 ^ t_reg2;//reg.store_data (RADDR(ptt[3]), t_reg1 ^ t_reg2);
		break;

	case OP_NOR  :
		t_reg1 = params[1];//reg.fetch_data(RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data(RADDR(ptt[2]));
		params[3] = ~(t_reg1 | t_reg2);//reg.store_data (RADDR(ptt[3]), ~(t_reg1 | t_reg2));
		break;

	case OP_SLT  :
		t_reg1 = params[1];//reg.fetch_data (RADDR(ptt[1]));
		t_reg2 = params[2];//reg.fetch_data (RADDR(ptt[2]));
		params[3] = (t_reg1 < t_reg2) ? 1:0; //reg.store_data (RADDR(ptt[3]), (t_reg1 < t_reg2)?1:0);
		break;

	case OP_ADDI :
		t_reg1 = params[2];//reg.fetch_data (RADDR(ptt[2])); //rs
		t_reg2 = params[3];
		params[1] = t_reg1 + t_reg2;//reg.store_data (RADDR(ptt[1]), t_reg1 + t_reg2);
		break;

	case OP_ANDI :
		t_reg1 = params[2];//reg.fetch_data (RADDR(ptt[2])); //rs
		t_reg2 = params[3];
		params[1] = t_reg1 & t_reg2; //reg.store_data (RADDR(ptt[1]), t_reg1 & t_reg2);
		break;

	case OP_ORI  :
		t_reg1 = params[2];//reg.fetch_data (RADDR(ptt[2])); //rs
		t_reg2 = params[3];
		params[1] = t_reg1 | t_reg2;//reg.store_data (RADDR(ptt[1]), t_reg1 | t_reg2);
		break;

	case OP_XORI :
		t_reg1 = params[2];//reg.fetch_data (RADDR(ptt[2])); //rs
		t_reg2 = params[3];
		params[1] = t_reg1 ^ t_reg2; //reg.store_data (RADDR(ptt[1]), t_reg1 ^ t_reg2);
		break;

	case OP_MOVZ :
		t_reg1 = params[1];//reg.fetch_data (RADDR(ptt[1])); //rt
		if (t_reg1 == 0)
			params[2]=params[3];//;reg.store_data (RADDR(ptt[2]), params[3]);
		break;
	}
	return ret;
}



/*
 * memory_access();
 * Access memory, refer to execute_instr for the way data i transffered
 */
uint32 instruction::memory_access()
{
        int ret=0;
	if (params[0] == OP_LW) {
		if (params[2] & 0x3)
			cout << "Unalligned Access";
		params[1]=ram.fetch_data(params[2]);//reg.store_data(RADDR(ptt[1]), ram.fetch_data(params[2]));
                ret = OP_LW;
	}
	else if (params[0] == OP_SW) {
		if (params[2] & 0x3)
			cout << "Unalligned Access";
		ram.store_data(params[2], reg.fetch_data(RADDR(ptt[1])));
                ret = OP_SW;
	}
        return ret;
}


/*
 *  execute_branch()
 *
 * fetch stage of the instruction
 * It is assumed PC=PC+<word_length> is done at fetch
 */
uint32 instruction::execute_branch()
{
	uint32 ret=0;
	int32 t_reg1, t_reg2;

	switch (params[0]) {
	case OP_SPECIAL:
		switch (params[1]) {

		case OP_SPECIAL_JR:
			reg.store_data(PC_ADDR, params[2]);
			break;

		case OP_SPECIAL_BREAK:
			ret = 1;
			break;

		case OP_SPECIAL_SLL: /*same for SLL*/
			if (instr == 0) { //NOP

			}
			break;
                default:
                        break;
                }
                break;

	case OP_REGIMM :
		switch (params[1]) {
		case OP_REGIMEM_BGEZ:
			t_reg1 = reg.fetch_data(RADDR(ptt[2]));
			if (t_reg1 >= 0) {
				t_reg2 = reg.fetch_data(PC_ADDR);
				t_reg2 = t_reg2 + params[3];
				reg.store_data(PC_ADDR, t_reg2);
			}
			break;
		case OP_REGIMEM_BLTZ:
			t_reg1 = reg.fetch_data(RADDR(ptt[2]));
			if (t_reg1 < 0) {
				t_reg2 = reg.fetch_data(PC_ADDR);
				t_reg2 = t_reg2 + params[3];
				reg.store_data(PC_ADDR, t_reg2);
			}
			break;
		}
		break;

	case OP_J:
		t_reg1 = reg.fetch_data(PC_ADDR);
		reg.store_data(PC_ADDR, params[1] | (t_reg1 & 0xF0000000));
		break;

	case OP_BEQ  :
		t_reg1 = reg.fetch_data (RADDR(ptt[1]));
		t_reg2 = reg.fetch_data (RADDR(ptt[2]));
		if (t_reg1 == t_reg2) {
			reg.store_data (PC_ADDR, 
					reg.fetch_data(PC_ADDR)+params[3]);
		}
		break;

	case OP_BNE :
		t_reg1 = reg.fetch_data (RADDR(ptt[1]));
		t_reg2 = reg.fetch_data (RADDR(ptt[2]));
		if (t_reg1 != t_reg2) {
			reg.store_data (PC_ADDR, 
					reg.fetch_data(PC_ADDR)+params[3]);
		}
		break;

	case OP_BGTZ :
		t_reg1 = reg.fetch_data (RADDR(ptt[1]));
		if (t_reg1 > 0) {
			reg.store_data (PC_ADDR, 
					reg.fetch_data(PC_ADDR)+params[2]);
		}
		break;

	case OP_BLEZ :
		t_reg1 = reg.fetch_data (RADDR(ptt[1]));
		if (t_reg1 <= 0) {
			reg.store_data (PC_ADDR, 
					reg.fetch_data(PC_ADDR)+params[2]);
		}
		break;

        default:
                break;

	}
	return ret;
}


/*
 * check_hazards()
 * Fill up the dest and src variables and then check for WAW, WAR and RAW
 * return the offset of the possible instruction
 */
#define WAW 1
#define RAW 2
#define WAR 3
#define MEM 4
int32 check_hazards(instruction *pre_issue) 
{
        int dest[PRE_ISSUE_BUFFER_SIZE];
        int src[PRE_ISSUE_BUFFER_SIZE][2];
        int res[PRE_ISSUE_BUFFER_SIZE];

        memset(res, 0, sizeof(int)*PRE_ISSUE_BUFFER_SIZE);

        for (int i=0 ; i < PRE_ISSUE_BUFFER_SIZE ; i++) {
                if ((pre_issue[i].in_use == true) && //is it valid
                    (pre_issue[i].last_processed < get_cycle())) // was it added in previous simulated cpu cycle
                        pre_issue[i].get_src_dest(&dest[i], &(src[i][0]));
        }

        for (int i=0 ; i < PRE_ISSUE_BUFFER_SIZE ; i++) {
                if ((pre_issue[i].in_use == true) && (pre_issue[i].last_processed < get_cycle())) {
                        if (dest[i] != -1 && (CHECK_REG_USAGE_ISSUE(dest[i]) != 0)) {
                                res[i] = WAW; //WAW with instruction already issued
                                continue;
                        }
                        if (src[i][0] != -1 && (CHECK_REG_USAGE_ISSUE(src[i][0]) != 0)) {
                                res[i] = RAW; //RAW with already issued instruction
                                continue;
                        }
                        if (src[i][1] != -1 && (CHECK_REG_USAGE_ISSUE(src[i][1]) != 0)) {
                                res[i] = RAW; //RAW with already issued instruction
                                continue;
                        }
                        for (int k=1 ; k < PRE_ISSUE_BUFFER_SIZE ; k++) {
                                int idx = (i+k) % PRE_ISSUE_BUFFER_SIZE; // with whom to compare

                                if (pre_issue[idx].in_use == false)
                                        continue;

                                if (pre_issue[i].fetch_cycle < pre_issue[idx].fetch_cycle  || pre_issue[i].in_use==false)
                                        continue; //no need to check as i is fetched before idx

                                if (pre_issue[idx].last_processed == get_cycle()) // we can't access this in this cycle
                                        continue;

                                if (dest[i] != -1 && (dest[i] == src[idx][0] || dest[i] == src[idx][1])) {
                                        res[i] = WAR;
                                        break;
                                }
                                if (dest[idx] != -1 && (src[i][0] == dest[idx] || src[i][1] == dest[idx])){
                                        res[i] = RAW;
                                        break;
                                }
                                if (dest[idx] != -1 && dest[idx]==dest[i]) {
                                        res[i] = WAW;
                                        break;
                                }
                                //check if earlied SW pendings
                                if (pre_issue[i].is_mem_instr() && pre_issue[idx].is_mem_instr() == OP_SW) {
                                        if (pre_issue[i].fetch_cycle > pre_issue[idx].fetch_cycle) {
                                                res[i] = MEM;
                                                break;
                                        }
                                }
                                
                        }
                }
        }

        for (int i=0; i < PRE_ISSUE_BUFFER_SIZE ; i++) {
                if ((pre_issue[i].in_use == true) && (pre_issue[i].last_processed < get_cycle())  && res[i]==0)
                        return i;
        }
        return -1;
}
/*
 * get_src_dest();
 * returns the source and destinations registers of an instruction
 */
void instruction::get_src_dest(int *dest, int *src)
{
	*dest = -1;
        *src = *(src+1) = -1;

	switch (params[0]) {
	case OP_SPECIAL:
		switch (params[1]) {

		case OP_SPECIAL_SLL:
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
                        *src = ptt[2];
                        *dest = ptt[3];
			break;
                default:
                        fprintf (stderr, "%d:Control Should not come here\n",__LINE__);
		}
		break;

	case OP_SW :
		*src = ptt[3];
                *(src+1) = ptt[1];
		break;

	case OP_LW :
		*src = ptt[3];
                *dest = ptt[1];
		break;

	case OP_ADD :
	case OP_SUB :
	case OP_MUL :
	case OP_AND :
	case OP_OR  :
	case OP_XOR :
	case OP_NOR  :
	case OP_SLT  :
		*src = ptt[1];
		*(src+1) = ptt[2];
		*dest = ptt[3];
		break;

	case OP_ADDI :
	case OP_ANDI :
	case OP_ORI  :
	case OP_XORI :
		*src = ptt[2]; //rs
		*dest = ptt[1];
		break;

	case OP_MOVZ :
		*src = ptt[1]; //rt
		*dest = ptt[2];
		break;
        default:
                fprintf (stderr, "%d,%s: Something has gone wrong check", __LINE__, str);
                break;
	}
}


/*
 ******* read_registers()********
 *read(actually update) the contents of register into the structure variables
 */
void instruction::read_registers(int update_register_map)
{
	switch (params[0]) {
		//this is the opcode for J, BREAK,SLL,SRL,SRA,NOP
	case OP_SPECIAL:
		switch(params[1]) {
			//SRL rd, rt, sa
			//SLL rd, rt, sa
			//SRA rd, rs, sa
		case OP_SPECIAL_SLL:
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
			//rt
			params[2] = reg.fetch_data(RADDR(ptt[2])); //rt

			//rd ptt[3]
                        if (update_register_map)
                                MARK_REG_USAGE_ISSUE(ptt[3]);

			break;
		}
		break;

		//SW rt, offset(base)
	case OP_SW :
	case OP_LW :
                //rt
		params[1] = reg.fetch_data(RADDR(ptt[1]));

                if (update_register_map && params[0]==OP_LW)
                        MARK_REG_USAGE_ISSUE(ptt[1]);

                //base
		params[3] = reg.fetch_data(RADDR(ptt[3]));
		break;

		//<inst> rd rs rt
	case OP_ADD  :
	case OP_SUB  :
	case OP_MUL  :
	case OP_AND  :
	case OP_OR   :
	case OP_XOR  :
	case OP_NOR  :
	case OP_SLT  :
		//rs
		params[1] = reg.fetch_data(RADDR(ptt[1]));

		//rt
		params[2] = reg.fetch_data(RADDR(ptt[2]));

		//rd - params[3]

                if (update_register_map)
                        MARK_REG_USAGE_ISSUE(ptt[3]);

		break;

		//ADD rt rs IMMEDIATE
	case OP_ADDI :
	case OP_ANDI :
	case OP_ORI  :
	case OP_XORI :
                //rt - for movz and to play safe 
		params[1] = reg.fetch_data(RADDR(ptt[1]));

                if (update_register_map)
                        MARK_REG_USAGE_ISSUE(ptt[1]);

	      	//rs
		params[2] = reg.fetch_data(RADDR(ptt[2]));
		break;

	case OP_MOVZ : //if rt==0 thrn rs=immediate
                //rt
                params[1] = reg.fetch_data(RADDR(ptt[1]));
                if (update_register_map)
                        MARK_REG_USAGE_ISSUE(ptt[2]);
                break;
        default:
                fprintf (stderr, "%d:something is wrong\n", __LINE__);
	}
}

/*
 * ******* write_back() *******
 *Write back to the register and update the issue and fetch register usage map
 */
uint32 instruction::write_back()
{
	switch (params[0]) {
	case OP_SPECIAL:
		switch (params[1]) {

		case OP_SPECIAL_SLL: 
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
			reg.store_data (RADDR(ptt[3]), params[3]);
                        UNMARK_REG_USAGE_ISSUE(ptt[3]);
                        UNMARK_REG_USAGE_FETCH(ptt[3]);
			break;
                default:
                        fprintf (stderr, "%s %s:%d Control shouldn't come here\n", str, __FUNCTION__, __LINE__);
                        break;
		}
		break;

	case OP_SW :
		break;

	case OP_LW :
                reg.store_data (RADDR(ptt[1]),params[1]);
                UNMARK_REG_USAGE_ISSUE(ptt[1]);
                UNMARK_REG_USAGE_FETCH(ptt[1]);
		break;

	case OP_ADD :
	case OP_SUB :	
	case OP_MUL :
	case OP_AND :
	case OP_OR  :
	case OP_XOR :
	case OP_NOR :
	case OP_SLT :
		reg.store_data (RADDR(ptt[3]), params[3]);
                UNMARK_REG_USAGE_ISSUE(ptt[3]);
                UNMARK_REG_USAGE_FETCH(ptt[3]);
		break;

	case OP_ADDI :
	case OP_ANDI :
	case OP_ORI  :
	case OP_XORI :
		reg.store_data (RADDR(ptt[1]), params[1]);
                UNMARK_REG_USAGE_ISSUE(ptt[1]);
                UNMARK_REG_USAGE_FETCH(ptt[1]);
		break;

	case OP_MOVZ :
                if (params[1]==0)
                        reg.store_data (RADDR(ptt[2]), params[2]);
                UNMARK_REG_USAGE_ISSUE(ptt[2]);
                UNMARK_REG_USAGE_FETCH(ptt[2]);
		break;
        default:
                fprintf(stderr, "%s %s:%d Control souldn't come here\n", str, __FUNCTION__, __LINE__);
                break;
	}
        return 0;
}
