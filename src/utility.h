#ifndef __PP_UTILITY__
#define __PP_UTILITY__

#include "type.h"

void print_usage ();

void read_cmdline_args(int argc, char *argv[]);

uint32 binstring_to_int(char *s);

void int_to_binstring(char *dest, uint32 num);

uint32 extend_sign_bit32(uint32 num, uint32 bit_pos);

uint32 get_cycle();

void inc_cycle();

#endif
