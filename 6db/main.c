#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mysql.h>

int main()
{
    MYSQL* mysql = mysql_init(NULL);

    mysql_close(mysql);
    return 0;
}

