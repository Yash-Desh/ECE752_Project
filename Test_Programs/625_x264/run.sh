#!/bin/bash

###### Parameters #####
pin_tool=dcache
benchmark=625_x264
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
../../Intel_Pin/pin -t ../../Intel_Pin/source/tools/Memory/obj-intel64/$pin_tool.so -- ./ldecod_s_base.mytest-m64 -i BuckBunny.264 -o BuckBunny.yuv | tee $benchmark.log