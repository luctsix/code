#include "../header.h"

int main()
{
    int fd[2];
    int ret = pipe(fd);

    if(ret == -1)
    {
        printf("errno=%d, %s", errno, strerror(errno));
    }

    long num = fpathconf(fd[0], _PC_PIPE_BUF);
    
    printf("num = %ld\n", num);

    return 0;
}

