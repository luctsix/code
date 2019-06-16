#!/bin/bash   

#函数

func()
{
    echo func is call
}

function func1
{
    echo func1 is call
}

functions func2
{
    echo func2 is called
}

echo before call

func 
func1
func2
echo end call

#参数和返回值
func1()
{
    echo "1 arg is:" $1
    echo "2 arg is:" $2
    echo "3 arg is:" $3
    echo "arg num is" $#

    echo "arg10 is:" ${10}

    return 1
}

#
func1 arg1 arg2 arg3 4 5 6 7 8 9 10 11
