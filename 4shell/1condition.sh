#!/bin/bash   

#逻辑判断

#数值条件
var1=100
var2=200

if [ $var1 -gt $var2 ]
then
    echo "var1 > var2"
else
    echo "var1 < var2"
fi

#字符比较
var1=abc
var2=abc

if [ $var1 = $var2 ]
then
    echo "var1 is var2"
else
    echo "var1 is not var2"
fi

#man test 查看
if [ -n $var1 ] 
#-n 字符串长度非零
#-z 字符串长度为0
then
    echo "var is not null"
else
    echo "var is  null"
fi


#短路
#前面返回0即成功，后面才能执行
ls a.txt && echo "exist"
#前面成功后面不执行
ls a.txt || exit 1

echo here
