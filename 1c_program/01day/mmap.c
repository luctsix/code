#include "../header.h"

int main()
{
    int fd = open("./test.txt", O_RDWR);
    int length = lseek(fd, 0, SEEK_END);
    if(fd == -1)
    {
        perror("open file ");
    }
    
    //匿名映射
    void *tmp = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS |  MAP_SHARED,
                     -1, 0);
    if(tmp != NULL)
    {
        printf("匿名映射成功，地址为：%p\n\n", tmp);
    }
    void* addr = mmap(
        NULL,
        4096,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
        );

    if(addr == NULL)
    {
        perror("mmap ");
        return 1;
    }

    printf("映射成功，地址为：%p\n", addr);

    int ret2 = munmap(addr, length);

    if(ret2 != -1)
    {
        printf("解除映射成功\n");
    }


    return 0;
}

