J(00000000000000000000010011)	/*Jump To 76*/	//TAKEN/*IF YOU DO NOT ANY NOP AND ALL OTHER CYCLE THEN YOU ARE GOOD*/
NOP()
NOP()
ADDI(R31,R0,0000000001011100) /*R31=92*/
JR(R31)	//Taken Goes to 1st BEQ below
J(00000000000000000000011000)//Goes to ADDI below	//Taken
NOP()
BEQ(R11,R10,1111111111111101)  /*goes to 2nd J from Top*/	//Taken
ADDI(R1,R0,0000000000000101) /*R1=5*/  
ADD(R2,R0,R1) /*R2=5*/
BEQ(R1,R0,0000000000000010) /*PC+8*/  	//N-TAKEN
ADDI(R0,R0,0000000000000000) 
BEQ(R1,R2,0000000000000011)  /*PC+12*/ 	//TAKEN
NOP()
NOP()
NOP()
BNE(R1,R2,0000000000000010) /*PC+8*/   	//N-TAKEN
ADDI(R0,R0,0000000000000000)/*DUMMY*/
BNE(R1,R0,0000000000000011) /*PC+12*/ 	//TAKEN
NOP()
NOP()
NOP()
ADDI(R3,R0,1111111111111000) /*R3 = -8*/
ADDI(R4,R0,0000000000000111) /*R4 = 7*/
BLTZ(R0,0000000000000010)  /*PC+8*/ 	//N-TAKEN
BLTZ(R3,0000000000000011)  /*PC+12*/ 	//TAKEN
NOP()
NOP()
NOP()
BGTZ(R0,0000000000000010)  /*PC+8*/	//N-TAKEN
BGTZ(R4,0000000000000011)  /*PC+12*/	//TAKEN
NOP()
NOP()
NOP()
BLEZ(R4,0000000000000001)  /*PC+4*/	//N-TAKEN
BLEZ(R0,0000000000000001)  /*PC+4*/	//TAKEN
NOP()
BLEZ(R3,0000000000000011)  /*PC+12*/	//TAKEN
NOP()
NOP()
NOP()
BGEZ(R3,0000000000000001)  /*PC+4*/	//N-TAKEN
BGEZ(R0,0000000000000001)  /*PC+4*/	//TAKEN
NOP()
BGEZ(R4,0000000000000011)  /*PC+12*/	//TAKEN
NOP()
NOP()
NOP()
BREAK()
