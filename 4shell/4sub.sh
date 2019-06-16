#!/bin/bash   

echo begin sub

echo $1
echo $2
#var=100
echo sub var is $var

#xargs
#xargs要和管道配合，用来将之前命令的输出的每一个元素，作为后面一个命令的参数
#进行执行。所以后面的命令有可能会执行很多次

echo ls a | xargs rm


echo end sub
