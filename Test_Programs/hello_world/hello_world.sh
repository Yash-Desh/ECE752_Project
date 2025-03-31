#!/bin/bash
cd ../../Intel_Pin/source/tools/Memory/
echo "Calling make clean in Memory Folder"
make clean
echo "Calling make in Memory Folder"
make
cd ../../../../Test_Programs/hello_world/
echo "Executing Pin Progam" 
../../Intel_Pin/pin -t /home/yash-desh/ECE752/ECE752_Project/Intel_Pin/source/tools/Memory/obj-intel64/dcache.so -- ./hello_world > hello_world.out