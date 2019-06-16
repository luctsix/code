#!/bin/bash   

#循环
files='ls'
echo $files

for file in $files
do
    echo $file
done

sum=0
var=1


while [ $var -le 100 ]
#while true
do
    sum='expr $sum + $var'
    var='expr $var + 1'

done
echo $sum

#分支
var=ok
case $var in
    ok)
        echo "ok"
        ;;
    notok)
        echo "not ok"
        ;;
    *)
        echo "$var"
        ;;
esac
