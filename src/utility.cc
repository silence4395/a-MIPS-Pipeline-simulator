#include <iostream>
#include <getopt.h>
#include <stdlib.h>

using namespace std;
#include "utility.h"
#include "type.h"


extern char *program_name, *input_file, *disasm_file, *sim_file;
extern int binary;

/*Keeps Track of current clock cycle*/
uint32 cycle_count=0;

uint32 get_cycle()
{
	return cycle_count;
}

void inc_cycle()
{
	cycle_count++;
}

/*
 * ***** print_usage() ****
 */
void print_usage ()
{
	cout << "Usage:\n" << program_name << " -I <input file> -D <disassembly file> -S <simulaton file>\n";
}

/*
 * ***** read_cmdline_args() ******
 */
void read_cmdline_args(int argc, char *argv[])
{
	const struct option long_options[] = {
                { "help",               0,NULL,'h'},
                { "input",              1,NULL,'I'},
                { "disasm",             1,NULL,'D'},
                { "sim",                1,NULL,'S'},
                { "binary",             1,NULL,'b'},
                { NULL,                 0,NULL,0}       //required for the end of array
        };
	const char short_options[]="hI:D:S:B:";
	int next_option;

	program_name = argv[0];
	do {
		next_option = getopt_long (argc, argv, short_options, long_options, NULL);
        
                switch (next_option) {
		case 'h':
			print_usage();
			break;
		case 'I':
			input_file = optarg;
			break;
		case 'D':
			disasm_file = optarg;
			break;
		case 'S':
			sim_file = optarg;
			break;
		case '?':
			print_usage();
			break;
		case -1:
			break;
		default:
			exit(1);
		}
	}while (next_option != -1);
}


/*
 * Used for sign bit extension, of a number
 * bit_pos is the position of th bit that should be extended to the right 
 * copy value of bit at bit_pos to all higher order bits compared to bit_pos
 *
 * bit_pos starts from 0,1,2.....31
 */
uint32 extend_sign_bit32(uint32 num, uint32 bit_pos)
{
	if(bit_pos >32)
		cout << __FUNCTION__ << "bit position can't be greater than 31\n";

	if((num & (1<<bit_pos)) == 0){ //if the bit at bit pos is zero
		while(bit_pos < 32) {
			num = num & ~(1<<bit_pos);
			bit_pos++;
		}
	}
	else {
		while (bit_pos < 32) {
			num = num | (1<<bit_pos);
			bit_pos++;
		}
	}
	return num;
}


uint32 binstring_to_int(char *s)
{
	uint32 a=(1<<31);
	uint32 res=0;
	while (a) {
		if (*s++ == '1')
			res |= a;
		a = a >> 1;
	}
	return res;
}

void int_to_binstring(char *dest, uint32 num)
{
	/*Print data in binary*/
	uint32 a=1<<31,i=0;
	while(a) {
		if (a & num) dest[i] = '1';
		else dest[i] = '0';
		a=a>>1; 
		i++;
	}
	dest[i] = NULL;
}
