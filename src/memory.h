#ifndef __PP_MEMORY__
#define __PP_MEMORY__

#include "type.h"

class memory{
	int32 *data;
	uint32 size;
	uint32 offset;
public:
	memory (int sz, int off);
	~memory();
	int32 fetch_data (uint32 address);
	void store_data (uint32 address, int32 data);
	uint32 get_size();
	void dump_memory(uint8 base);
};
#endif
