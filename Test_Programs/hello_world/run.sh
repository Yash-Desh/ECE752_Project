#!/bin/bash

###### Parameters #####
pin_tool=mycache_L1
benchmark=hello_world
# No data/all folder
#######################

cd ../../Intel_Pin/source/tools/Memory/obj-intel64
echo "Deleting Object Files from obj-intel64 folder"
rm $pin_tool.so $pin_tool.o
cd ..
echo "Calling make in Memory Folder"
make
cd ../../../../Test_Programs/$benchmark/
echo "Executing Pin Progam" 
../../Intel_Pin/pin -t ../../Intel_Pin/source/tools/Memory/obj-intel64/$pin_tool.so -- ./$benchmark | tee $benchmark.log