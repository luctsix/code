#include "../header.h"

int main()
{
    int ret = mkfifo("./FIFO", 0777);
    if(ret == -1)
    {
        perror("mkfifo:");
    }

    pid_t pid = fork();

    if(pid > 0)
    {
        printf("i am father, now i am writing into FIFO...\n");
        int fd1 = open("./FIFO", O_WRONLY);
        if(fd1 < 0)
        {
            perror("open write_FIFO ");
        }
        char buf[1024] = "hello, zhu";
        write(fd1, buf, sizeof(buf));
        close(fd1);
    }
    else if(pid == 0)
    {
        printf("\n------------\n");
        printf("i am child, now i am reading from FIFO...\n");
        int fd2 =  open("./FIFO", O_RDONLY);
        if(fd2 < 0 )
        {
            perror("open read_FIFO ");
            return 1;
        }
        char buf[1024] = {0};
        read(fd2, buf, sizeof(buf));
        printf("buf = %s\n", buf);
        close(fd2);
    }

    return 0;
}

