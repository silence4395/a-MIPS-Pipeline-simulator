/* -*- linux-c -*-
 * main.cc 
 */

#include<iostream>
#include<fstream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>

#include "memory.h"
#include "utility.h"
#include "type.h"
#include "instructions.h"

using namespace std;

//Size in terms of words(4 byte)
#define RAM_SIZE (256*1024)
/*32 registers + PC*/
#define REGFILE_SIZE (32+1)

char *program_name, *input_file, *disasm_file, *sim_file;
int binary=0;

/*Our RAM size will be RAM_SIZEx4 bytes and starting at 64*/
memory ram(RAM_SIZE,64);
/*Our Register file size 32+1 4-byte words..... last word is Program Counter*/
memory reg(REGFILE_SIZE,0);

/*pre-issue buffer*/
instruction pre_issue[PRE_ISSUE_BUFFER_SIZE];
/*pre-alu buffer*/
instruction pre_alu[PRE_ALU_BUFFER_SIZE];
/*post-alu buffer*/
instruction post_alu[POST_ALU_BUFFER_SIZE];
/*post-memory buffer*/
instruction post_mem[POST_MEMORY_BUFFER_SIZE];

/*max_address - Maximum address initialized
 *data_address - point of first break
 * Even though we do not treat data/code as different in memory, this is required for generating simulator output
 */
uint32 max_address=0, data_address=0;


/*
 ****************initialize_ram()*********************
 *Function 1. Initialize ram using the input file
 *Function 2: During the process generate the disassembly file.
 */
uint32 initialize_ram()
{
	ifstream in;
	ofstream out;
	char s[32+2];
	uint32 address=64;
	int stop_disasm=0;
        static char default_file_name[]="/dev/null";

        if (!disasm_file)
                disasm_file=default_file_name;

	in.open (input_file, ios::in);
	out.open (disasm_file, ios::out|ios::trunc);

	if (!in || !out) {
		cout << "Cannot open file " << input_file << "\n";
		exit(1);
	}

	while (in) {
		instruction instr;
		s[0]='\0';
		in >> s;
		if (strlen(s)!=32)
			break;
		else {
			ram.store_data(address, binstring_to_int(s));
			address+=4;
		}

		/*The below if else branch does the disassembly as well :)*/
		if (!stop_disasm) {
			out << s[0] << " ";
			out << s[1] << s[2] << s[3] << s[4] << s[5] << " ";
			out << s[6] << s[7] << s[8] << s[9] << s[10] << " ";
			out << s[11] << s[12] << s[13] << s[14] << s[15] << " ";
			out << s[16] << s[17] << s[18] << s[19] << s[20] << " ";
			out << s[21] << s[22] << s[23] << s[24] << s[25] << " ";
			out << s[26] << s[27] << s[28] << s[29] << s[30] << s[31] << "\t";
			out << address-4 << "\t";
	
			stop_disasm = instr.disassemble_instr (binstring_to_int(s), 0);
			out << instr.str << "\n";
			
			if (stop_disasm)
				data_address = address;
		}
		else {
			out << s << "\t" << address-4 << "\t" << (int32)binstring_to_int(s)<< "\n";
		}
                if (address > RAM_SIZE*4) {
                        cout << "Hey your code seems quite BIG!!!!\n Increase the size of the macro RAM_SIZE in main.cc.Thanks!!\n";
                }
	}

        if (!stop_disasm) {
                cout << "Break not Found!!!!!!!\n";
                cout << "you might want to check the input\nAnyways i ll continue ;)\n";
        }
	in.close();
	out.close();

	return (address-4);
}

//fetch decode will update the below values
extern char wait_inst[], exec_inst[];

void print_mem_status(ofstream &out) 
{
        unsigned int i;
	out << "--------------------\n";
	out << "Cycle:" << get_cycle() <<"\n\n";

        out << "IF Unit:\n";
        out << "\tWaiting Instruction: " << wait_inst <<"\n";
        out << "\tExecuted Instruction: " << exec_inst <<"\n";
        //---------------------------
        out << "Pre-Issue Buffer:\n";
        out << "\tEntry 0:";
        if (pre_issue[0].in_use) 
                out<< "[" << pre_issue[0].str << "]\n";
        else
                out << "\n";

        out << "\tEntry 1:";
        if (pre_issue[1].in_use) 
                out<< "[" << pre_issue[1].str << "]\n";
        else
                out << "\n";

        out << "\tEntry 2:";
        if (pre_issue[2].in_use) 
                out<< "[" << pre_issue[2].str << "]\n";
        else
                out << "\n";
        //-----------------------------
        out << "Pre-ALU Queue:\n";
        out << "\tEntry 0:";
        if (pre_alu[0].in_use) 
                out<< "[" << pre_alu[0].str << "]\n";
        else
                out << "\n";

        out << "\tEntry 1:";
        if (pre_alu[1].in_use) 
                out<< "[" << pre_alu[1].str << "]\n";
        else
                out << "\n";
        //-----------------------------

        out << "Post-ALU Buffer:";
        if (post_alu[0].in_use) 
                out<< "[" << post_alu[0].str << "]\n";
        else
                out << "\n";
        //----------------------------
        out << "Post-MEM Buffer:";
        if (post_mem[0].in_use) 
                out<< "[" << post_mem[0].str << "]\n";
        else
                out << "\n";
        //----------------------------

	out << "\nRegisters";

	out << "\nR00:";
	i=0;
	while (i<8) {
		out << "\t" << (int)reg.fetch_data(RADDR(i));
		i++;
	}
	out << "\nR08:";
	while (i<16) {
		out << "\t" << (int)reg.fetch_data(RADDR(i));
		i++;
	}
	out << "\nR16:";
	while (i<24) {
		out << "\t" << (int)reg.fetch_data(RADDR(i));
		i++;
	}
	out << "\nR24:";
	while (i<32) {
		out << "\t" << (int)reg.fetch_data(RADDR(i));
		i++;
	}

	out << "\n\nData\n";
	i=data_address;
	while(i <= max_address) {
		out << i << ":\t" << (int)ram.fetch_data(i); i+=4;
                if (i <= max_address)	out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		if (i <= max_address)    out << "\t" << (int)ram.fetch_data(i); i+=4;
		out << "\n";
	}

	out.flush();
}


/*
 *check for pending buffers and 
 *return 1 if pipeline is clear(no pending buffers)
 * else 0 (pending buffers)
 */
int pipeline_clear ()
{
        for (int i=0; i < PRE_ISSUE_BUFFER_SIZE ;i++) {
                if (pre_issue[i].in_use)
                        return 0;
        }

        for (int i=0; i < PRE_ALU_BUFFER_SIZE ;i++) {
                if (pre_alu[i].in_use)
                        return 0;
        }

        for (int i=0 ; i<POST_ALU_BUFFER_SIZE ; i++) {
                if (post_alu[i].in_use)
                        return 0;
        }

        for (int i=0 ;i < POST_MEMORY_BUFFER_SIZE ;i++) {
                if (post_mem[i].in_use)
                        return 0;
        }

        return 1;
}


void initiate_pipeline ()
{
	ofstream out;
	uint32 no_break=0;

	out.open(sim_file, ios::out | ios::trunc);
	if (!out) {
		cout << "error opeaning file sim_file\n";
		exit(1);
	}
	reg.store_data(PC_ADDR, 64);

        instruction l_post_alu[POST_ALU_BUFFER_SIZE];
        instruction l_post_mem[POST_MEMORY_BUFFER_SIZE];


        /*Initialize all pipeline buffers*/
        for (int i=0; i < PRE_ISSUE_BUFFER_SIZE ;i++)
                pre_issue[i].in_use = 0;
        for (int i=0; i < PRE_ALU_BUFFER_SIZE ;i++)
                pre_alu[i].in_use = 0;
        for (int i=0; i< POST_ALU_BUFFER_SIZE;i++) {
                post_alu[i].in_use = 0;
                l_post_alu[i].in_use = 0;
        }
        for (int i=0; i< POST_MEMORY_BUFFER_SIZE;i++){
                post_mem[i].in_use = 0;
                l_post_mem[i].in_use = 0;
        }

        inc_cycle();

	while (1) {
                //readin
                memcpy (l_post_alu, post_alu, sizeof(instruction)*POST_ALU_BUFFER_SIZE);
                memcpy (l_post_mem, post_mem, sizeof(instruction)*POST_MEMORY_BUFFER_SIZE);

                if (!no_break) 
                        no_break = fetch_decode (pre_issue);
                else {
                        wait_inst[0]='\0';
                        exec_inst[0]='\0';
                }

                //invalidate before execution
                post_alu[0].in_use=false;
                execute_stage (pre_alu, post_alu);
                issue_stage (pre_issue, pre_alu);

                writeback_stage (l_post_alu, post_mem);
                memory_stage (l_post_alu , post_mem);

                //print data here
                print_mem_status(out);

                inc_cycle();
                if (no_break && pipeline_clear())
                        break;
                //check all buffers. if all in_use set to zero break out of loop
	}
	out.close();
}
/*
 * ***** main() *****
 */
int main(int argc, char*argv[])
{
#if 0
	read_cmdline_args(argc, argv);
#else
        if (argc != 3){
                fprintf (stderr, "Wrong input\n");
                exit(1);
        }
        input_file = argv[1];
        sim_file = argv[2];
#endif
	if (!input_file || !sim_file) {
		print_usage();
		exit(1);
	}

	max_address = initialize_ram();
	initiate_pipeline();
	return 0;
}
