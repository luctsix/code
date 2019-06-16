#!/bin/bash   
hi, tom, i am john.
do you know what is his name.
which is your name, 010-11112222 or 0756-22221111
my email is hi@xueguoliang.cn, what is yours.
my email is xxx@163.com, his is yyy@yahoo.cc
this is my website http://xueguoliang.cn
which site are you reading?
oh, it is https://www.51job.com/index.html

#正则表达式
```
#选择
|           (a|b)   搜索a或b

#贪婪       + * ...  
#懒惰       ?
*?          重复任意次，但尽可能少重复
+?          重复1次或更多次，但尽可能少重复
??          重复0次或1次，但尽可能少重复
{n,m}?      重复n到m次，但尽可能少重复
{n,}?       重复n次以上，但尽可能少重复

#通配符
.           匹配除换行符意外的任意字符
\w          匹配字母或数字或下划线或汉字
\s          匹配任意的空白符
\d          匹配数字，在grep中要使用-P参数才能使用该元字符
\b          匹配单词的开始或结束    
^           匹配字符串的开始
$           匹配字符串的结束
\W          匹配任意不是字母，数字，下划线，汉字的字符
\S          匹配任意不是空白符的字符
\D          匹配任意非数字的字符
\B          匹配不是单词开头或结束的位置
[a-z]       匹配a到z的任意一个字符
[0-9]       匹配数组0-9
[acek-i]    匹配一些字母
[^x]        匹配除了x以外的任意字符
[^aeiou]    匹配除了aeiou这几个字母意外的任意字符

#重复符
+           重复零次或多次
*           重复一次或多次
？          重复零次或一次
{n}         重复n次
{n,}        重复n次或更多次
{n,m}       重复n次到m次    

```

var is: 50 

