#!/bin/bash
cd ../../Intel_Pin/source/tools/Memory/
echo "Calling make clean in Memory Folder"
make clean
echo "Calling make in Memory Folder"
make
cd ../../../../Test_Programs/602_gcc/
echo "Executing Pin Progam" 
../../Intel_Pin/pin -t ../../Intel_Pin/source/tools/Memory/obj-intel64/allcache.so -- ./sgcc_base.mytest-m64 200.c -O3 -finline-limit=50000  > 602_gcc.out