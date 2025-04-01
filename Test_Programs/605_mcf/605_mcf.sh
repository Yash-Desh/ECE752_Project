#!/bin/bash
cd ../../Intel_Pin/source/tools/Memory/
echo "Calling make clean in Memory Folder"
make clean
echo "Calling make in Memory Folder"
make
cd ../../../../Test_Programs/605_mcf/
echo "Executing Pin Progam" 
../../Intel_Pin/pin -t ../../Intel_Pin/source/tools/Memory/obj-intel64/dcache.so -o dcache_605_mcf.out -- ./mcf_s_base.mytest-m64 inp.in > 605_mcf.out