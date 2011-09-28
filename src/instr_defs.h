#ifndef __PP__INST_DEFS__
#define __PP_INST_DEFS__

#define PRE_ISSUE_BUFFER_SIZE 3
#define PRE_ALU_BUFFER_SIZE 2
#define POST_ALU_BUFFER_SIZE 1
#define POST_MEMORY_BUFFER_SIZE 1

/****************About Opcodes***********************/
#define OPCODE_WIDTH 6
#define INSTRUCTION_WIDTH 32

/***********************OPCODES************************/
#define OP_SPECIAL 0x00
#define OP_REGIMM  0x01
/*Opcodes for category-1 instruction*/
#define OP_J     0x02 
#define OP_JR    OP_SPECIAL 
#define OP_BEQ   0x04
#define OP_BNE   0x05 
#define OP_BLTZ  OP_REGIMM
#define OP_BGTZ  0x07
#define OP_BLEZ  0x06 
#define OP_BGEZ  OP_REGIMM
#define OP_BREAK OP_SPECIAL
#define OP_SW    0x0B
#define OP_LW    0x03
#define OP_SLL   OP_SPECIAL
#define OP_SRL   OP_SPECIAL
#define OP_SRA   OP_SPECIAL
#define OP_NOP   OP_SPECIAL

/*Opcodes for category 2 instructions*/
#define OP_ADD   0x23
#define OP_SUB   0x22
#define OP_MUL   0x26
#define OP_AND   0x29
#define OP_OR    0x2D
#define OP_XOR   0x2A
#define OP_NOR   0x21
#define OP_SLT   0x2B

/*Opcodes for category 3 instruction*/
#define OP_ADDI  0x30
#define OP_ANDI  0x32
#define OP_ORI   0x33
#define OP_XORI  0x35
#define OP_MOVZ  0x36


/****************************/
#define OP_SPECIAL_JR    0x08
#define OP_SPECIAL_BREAK 0x0D
#define OP_SPECIAL_SLL   0x00
#define OP_SPECIAL_SRL   0x02
#define OP_SPECIAL_SRA   0x03
/*sa is zero in the NOP case*/
#define OP_SPECIAL_NOP   0x00

#define OP_REGIMEM_BGEZ 0x01
#define OP_REGIMEM_BLTZ 0x00

/************************Parameter types****************************/
enum param_type_t {
	R0=0,
	R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,
	R11,R12,R13,R14,R15,R16,R17,R18,R19,R20,
	R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,
	R31,PC,
	SHIFT,
	IMMEDIATE5,
	IMMEDIATE16,
	ADDRESS18,
	ADDRESS28,
	OPCODE
};

#define PC_ADDR (32 * 4)
#define RADDR(A) (A*4)

#endif
