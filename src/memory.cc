#include <iostream>
#include <stdlib.h>
#include <cstdio>

using namespace std;

#include "type.h"
#include "memory.h"
#include "utility.h"

/*Number of words to allocate*/
memory :: memory(int sz, int off)
{
	offset = off;
	try {
		data = new int32[sz];
	} catch (bad_alloc xa) {
		cout << "Memory Allocation Failure\n";
		exit(1);
	}
	size = sz;
}

memory :: ~memory( )
{
	delete data;
}

int32 memory :: fetch_data (uint32 address)
{
	if (address & 0x3) {
		cout << "Unalligned memory access\n";
		exit(1);
	}

	if (address > (size*4 + offset)) {
		cout << "Accessing memory that does not exist\n";
		exit(1);
	}

	if (address < offset) {
		cout << "Accessing address < " << offset <<"\n";
		exit(1);
	}

	address = (address-offset) / 4;

	return data[address];
}

void memory :: store_data (uint32 address, int32 cont)
{
	if (address & 0x3) {
		cout << "Unalligned memory access\n";
		exit(1);
	}

	if (address > (size*4 + offset)) {
		cout << "Accessing memory that does not exist\n";
		exit(1);
	}

	if (address < offset) {
		cout << "Accessing address < " << offset <<"\n";
		exit(1);
	}

	address = (address-offset) / 4;

	data[address] = cont;
}

/*
 * return memory allocated 
 */
uint32 memory::get_size()
{
	return size*4;
}

void memory::dump_memory(uint8 base)
{
	unsigned int i=0;
	if (base == 16) {
		while (i < size) {
			cout <<  hex << data[i] <<"\n";
			i++;
		}
	}
	else if (base == 2) {
		while (i<size) {
			/*Print data in binary*/
			uint32 a=1<<31;
			while(a) {
				if (a & data[i]) cout << "1";
				else cout << "0";
				a=a>>1; 
			}
			cout << "\n";
			i++;
		}
	}
	else {
		cout << __FUNCTION__ << " : base " << base << " not supported\n";
	}
}
