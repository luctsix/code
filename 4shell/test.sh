#!/bin/bash   

#第一行注释用于指定shell的种类，只能写在第一行
echo $HOME

#自定义变量
var=100
echo $valllo
echo ${val}llo

#h环境变量
echo $HOME

#定义环境变量
export MyEnvVar=aaa
echo $MyEnvVar

#环境变量和自定义变量冲突时，环境变量被覆盖

expr 1 + 1   #加
expr 3 \* 2   #乘
expr 6 / 2
