#!/bin/sh
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/yiyang/zzy/redis/lib"
ulimit -c unlimited
./test 
