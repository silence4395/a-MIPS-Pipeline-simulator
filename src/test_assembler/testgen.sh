#!/bin/sh
#include the file
echo "#include \"test_gen_reg.h\"" > __$1
cat $1 >> __$1
#gen preprocessing o/p
gcc -E __$1 > ___$1
#include the file
echo "#include \"test_gen_inst.h\"" > __$1 
cat ___$1 >> __$1
#gen_pre_processing_out
gcc -E __$1 | grep 0 > $1".out"
rm __*