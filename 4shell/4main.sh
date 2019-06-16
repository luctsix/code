#!/bin/bash   

#export var=50  #声明成环境变量后可以给子进程传递
var=60
#执行子脚本

echo begin 'main->sub'
#./4sub.sh
#. 4sub.sh #使用点+空格执行脚本时，这个bash没有进行fork，而是直接使用当前bash来执行脚本文件

source 4sub.sh  arg1 arg2     #使用source和使用点空一样
echo $?

echo main var is $var
echo end 'main->sub'

