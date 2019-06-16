#include "../header.h"

extern int errno;

int main()
{
    int fd = open("test.txt", O_WRONLY | O_CREAT, 0777);

    if(fd < 0)
    {
        printf("errno = %d,  %s", errno, strerror(errno));
        return 1;
    }
    char buf[20] = "hello, zhuzhu";
    write(fd, buf, sizeof(buf));

    close(fd);

    return 0;
}

