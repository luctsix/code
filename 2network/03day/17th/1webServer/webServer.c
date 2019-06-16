#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <dirent.h>

#include "pub.h"

#define PORT 10086
#define MAXSIZE 128
#define SIZE 4096

/*
1. 创建套接字(socket)

2. 初始化结构体 

3. 绑定 bind

4. 监听 listen

5. 创建epoll专用的文件描述符 epoll_create

6. sockfd上树 epoll_ctl

7. 循环监视 epoll_wait

8. 关闭文件描述符 close

*/

//发送响应报文头部
int send_header(int connfd, int code, char *msg, char *fileType, int fileLength)
{
    int ret = -1;
    char buf[MAXSIZE];

    //1. 发送状态行
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "HTTP/1.1 %d %s\r\n", code, msg);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send"); 
        goto err0;
    }

    //2. 发送文件类型
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "Content-Type:%s\r\n", fileType);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send"); 
        goto err0;
    }

    //3. 发送数据长度
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "Content-Length:%d\r\n", fileLength);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send"); 
        goto err0;
    }

    //4. 发送连接状态
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "Connection:close\r\n");
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send"); 
        goto err0;
    }
    //5. 发送空行
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "\r\n");
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send"); 
        goto err0;
    }

    return 0;
err0:
    return -1;
}

//发送目录内容
int send_dir(int connfd, char *fileName)
{
    int ret = -1;
    char buf[SIZE];
    DIR *dir = NULL;

    struct dirent *d = NULL;

    //1. 打开目录
    dir = opendir(fileName);
    if (NULL == dir)
    {
        perror("opendir"); 
        return 1;
    }


    //2. 循环读取目录内容
    while(1)
    {
        d = readdir(dir); 
        if (NULL == d)
        {
            break; 
        }
    

        memset(buf, 0, SIZE);
        if (d->d_type == DT_REG)
        {
            sprintf(buf, "<li><a href='%s'>%s</a></li>", d->d_name, d->d_name);
        }
        else  if (d->d_type == DT_DIR)
        {
            sprintf(buf, "<li><a href='%s/'>%s</a></li>", d->d_name, d->d_name);
        }
        ret = send(connfd, buf, strlen(buf), 0); 
        if (ret <= 0)
        {
            perror("send");
            break; 
        }
    }

    //3. 关闭目录
    closedir(dir);
    return 0;
}

//发送文件的内容
int send_file(int connfd, char *fileName)
{
    int ret = -1;
    int fd = -1;

    char buf[SIZE];

    fd = open(fileName, O_RDONLY);
    if (-1 == fd)
    {
        perror("open"); 
        goto err0;
    }

    //读取文件内容 然后发送
    while(1)
    {
        memset(buf, 0, SIZE);

        ret = read(fd, buf, SIZE);
        if (ret <= 0)
        {
            perror("read"); 
            break;
        }
    
        //ret = send(connfd, buf, strlen(buf), 0);
        ret = send(connfd, buf, ret, 0);
        if (ret <= 0)
        {
            perror("send"); 
            break;
        }
    }


    //关闭文件
    close(fd);

    return 0;
err0:
    return -1;
}

//解析http请求报文
int http_request(int epollfd, int connfd)
{
    int ret = -1;
    char str[MAXSIZE];
    char buf[MAXSIZE];

    char method[32];
    char url[32];
    char version[32];

    struct stat s;

    char *pfile = NULL;

    //获取请求行
    memset(str, 0, MAXSIZE);
    ret = get_line(connfd, str, MAXSIZE); 
    if (ret <= 0)
    {
        perror("recv");
        return -1;
    }
    printf("%s", str);

    sscanf(str, "%s %s %s\r\n", method, url, version);
    printf("method:%s url: %s version: %s\n", method, url, version);

    //解码
    strdecode(url, url); 
    printf("method:%s url: %s version: %s\n", method, url, version);

    //获取请求头标
    while(1)
    {
        memset(buf, 0, MAXSIZE);
        ret = get_line(connfd, buf, MAXSIZE); 
        if (ret <= 0)
        {
            perror("recv");
            break; 
        }
        printf("%s", buf);
    
    }

    printf("get all msg ok....\n");

    //判断是否为get请求
    if (strncmp(method, "GET", 3) != 0)
    {
        printf("不是get请求...\n"); 
        return -1;
    }

    //去掉/  /
    pfile = url + 1;
    if ('\0' == *pfile)
    {
        pfile = "./"; 
    }

    //获取文件相关信息
    memset(&s, 0, sizeof(s));
    ret = stat(pfile, &s);
    if (-1 == ret)
    {
        //发送错误的页面
        //发送响应报文首部
        send_header(connfd, 404, "NOT fOUND", get_mime_type("*.html"), -1);

        //发送错误页面
        send_file(connfd, "error.html");

        return -1;
    }

    //判断是否为文件
    if (S_ISREG(s.st_mode))
    {
        //发送文件的内容
         
        //发送响应报文首部
        send_header(connfd, 200, "OK", get_mime_type(pfile), s.st_size);    

        //发送文件内容
        send_file(connfd, pfile);
    }
    else if (S_ISDIR(s.st_mode))
    {
        //发送目录的内容
        send_header(connfd, 200, "OK", get_mime_type("*.html"), -1);
        //发送html文档上半部分
        send_file(connfd, "dir_header.html");

        //发送目录内容
        send_dir(connfd, pfile);

        //发送html文档下半部分
        send_file(connfd, "dir_tail.html");

    }
     


    return 0;
}

int main(void)
{
    int i = 0;
    int ret = -1;
    int count = -1;
    int sockfd = -1;
    int connfd = -1;

    int epollfd = -1;

    struct sockaddr_in addr;
    struct sockaddr_in from;

    struct epoll_event e;
    struct epoll_event re[MAXSIZE];

    socklen_t len = -1;

    //改变当前进程工作目录为/home/deng/html
    ret = chdir(getenv("HOME")); 
    if (-1 == ret)
    {
        perror("chdir"); 
        goto err0;
    }

    ret = chdir("html");
    if (-1 == ret)
    {
        perror("chdir");
        goto err0;
    }

    //获取当前进程工作目录
    system("pwd");

    //1. 创建套接字(socket)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);    
    if (-1 == sockfd)
    {
        perror("socket"); 
        goto err0;
    }

    //设置端口复用
    i = 1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &i, sizeof(i));
    if (-1 == ret)
    {
        perror("setsockopt"); 
        goto err1;
    }

    //2. 初始化结构体 
    memset(&addr, 0, sizeof(addr));    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    //3. 绑定 bind
    ret = bind(sockfd, (void*)&addr, sizeof(addr));    
    if (-1 == ret)
    {
        perror("bind"); 
        goto err1;
    }

    //4. 监听 listen
    ret = listen(sockfd, 10);     
    if (-1 == ret)
    {
        perror("listen"); 
        goto err1;
    }
    printf("服务端处于监听状态.....\n");

    //5. 创建epoll专用的文件描述符 epoll_create
    epollfd = epoll_create(1024);    
    if (-1 == epollfd)
    {
        perror("epoll_create"); 
        goto err1;
    }

    //6. sockfd上树 epoll_ctl
    memset(&e, 0, sizeof(e));    
    e.events = EPOLLIN;
    e.data.fd = sockfd;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &e);
    if (-1 == ret)
    {
        perror("epoll_ctl"); 
        goto err2;
    }

    //7. 循环监视 epoll_wait
    while(1)    
    {
        memset(re, 0, sizeof(re));
        ret = epoll_wait(epollfd, re, MAXSIZE, 3000);
        if (-1 == ret)
        {
            perror("epoll_wait"); 
            break;
        }
        else if (0 == ret)
        {
            printf("3 seconds timeout....\n"); 
            continue;
        }
        else
        {
            //表示有多少文件描述符准备好了
            count = ret;
            for (i = 0; i < count; i++) 
            {
                //表示读事件
                if (re[i].events & EPOLLIN)
                {
                    //表示有新的客户端连接
                    if (sockfd == re[i].data.fd)
                    {
                        memset(&from, 0, sizeof(from));
                        len = sizeof(from);
                        connfd = accept(sockfd, (void*)&from, &len);
                        if (-1 == connfd)
                        {
                            perror("accept");
                            continue;
                        }
                        
                        printf("\033[31m客户端%s:%d连接到服务端\033[0m\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));

                        //获取当前标志
                        ret = fcntl(connfd, F_GETFL);
                        ret = ret | O_NONBLOCK;
                        fcntl(connfd, F_SETFL, ret);

                        //将新的客户端加入监视的集合中
                        memset(&e, 0, sizeof(e));                    
                        //设置为边沿触发
                        e.events = EPOLLIN | EPOLLET;
                        e.data.fd = connfd;

                        ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &e);
                        if (-1 == ret)
                        {
                            perror("epoll_ctl"); 
                            close(connfd);
                            continue;
                        }

                    }
                    else
                    {
                        //有新的客户端请求数据  读取数据
                        http_request(epollfd, re[i].data.fd);
                    }
                
                
                }
            }
        }
    }

    //8. 关闭文件描述符 close
    close(sockfd);
    close(epollfd);

    return 0;

err2:
    close(epollfd);
err1:
    close(sockfd);
err0:
    return 1;
}
