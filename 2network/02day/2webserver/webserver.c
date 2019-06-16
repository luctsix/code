#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "pub.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define SIZE 128
#define PORT 10086
#define MAX  1024

//int get_line(int sock, char *buf, int size)
//读一行
//char *get_mime_type(char *name)
//获取文件类型
//

int send_header(int connfd, int code, char* msg, char* fileType, int fileLength)
{
    printf("now int function: %s....\n", __FUNCTION__);
    int ret = -1;
    char buf[MAX];

    memset(buf, 0, MAX);
    sprintf(buf, "HTTP/1.1 %d %s\r\n", code, msg);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send");
        goto err0;
    }
    

    memset(buf, 0, MAX);
    sprintf(buf, "Content_Type:%s\r\n", fileType);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send");
        goto err0;
    }

    memset(buf, 0, MAX);
    sprintf(buf, "Content_Length:%d\r\n", fileLength);
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send");
        goto err0;
    }

    memset(buf, 0, MAX);
    sprintf(buf, "Connection:close\r\n");
    ret = send(connfd, buf, strlen(buf), 0);
    if (ret <= 0)
    {
        perror("send");
        goto err0;
    }

    memset(buf, 0, MAX);
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

int send_dir(int connfd, char* fileName)
{
    int ret = -1;
    DIR* dir = NULL;
    struct dirent* d = NULL;
    char buf[MAX];

    printf("/n--------cutting line---------\n");
    printf("now is here:%s filename=%s connfd=%d\n", __FUNCTION__, fileName, connfd);

    dir = opendir(fileName);
    if (NULL == dir)
    {
        perror("opendir");
        return 1;
    }

    while(1)
    {
        d = readdir(dir);
        if( NULL == d)
        {
            break;
        }

        memset(buf, 0, MAX);
        if(d->d_type == DT_DIR)
            sprintf(buf, "<li><a href='%s/'>%s</a></li>", d->d_name, d->d_name);
        else if(d->d_type == DT_REG)
            sprintf(buf, "<li><a href=%s>%s</a></li>", d->d_name, d->d_name);

        ret = send(connfd, buf, strlen(buf), 0);
        if(ret <= 0)
        {
            perror("send");
            break;
        }
    }

    closedir(dir);
}

int send_file(int connfd, char* fileName)
{
    printf("\n----------cutting line---------\n");
    printf("now is here: %s...\n", __FUNCTION__);
    printf("filename: %s \n", fileName);
    int ret = -1;
    int fd  = -1;

    char buf[MAX], str[MAX];

    fd = open(fileName, O_RDONLY);
    if(-1 == fd)
    {
        perror("open");
        goto err0;
    }

    while(1)
    {
        memset(buf, 0, MAX);
        ret = read(fd, buf, MAX);
        if(ret <= 0)
        {
            perror("read");
            break;
        }
        ret = send(connfd, buf, ret, 0);
        if(ret <= 0)
        {
            perror("send");
            break;
        }
    }

    //close(connfd);
    return 0;
err0:
    return -1;
}

int http_request(int connfd)
{
    int ret = -1;
    char str[MAX];
    char buf[MAX];

    char method[32];
    char url[32];
    char version[32];

    struct stat s;

    char *pfile = NULL;

    memset(str, 0, MAX);
    ret = get_line(connfd, str, MAX);
    if( ret <= 0)
    {
        perror("recv");
        return -1;
    }

    printf("%s\n", str);

    sscanf(str, "%s %s %s\r\n", method, url, version);
    printf("method:%s url:%s version:%s\n", method, url, version);

    strdecode(url, url);

    while(1)
    {
        memset(buf, 0, MAX);
        ret = get_line(connfd, buf, MAX);
        if (ret -1 <= 0)
        {
            perror("recv");
            break;
        }
        printf("%s", buf);
    }

    printf("get all msg ok....\n");

    if(strncmp(method, "GET", 3) != 0)

    {
        printf("不是get请求...\n");
        return -1;
    }

    pfile = url + 1;    // 去掉 /
    if ('\0' == *pfile)
    {
        pfile = "./";
    }

    memset(&s, 0, sizeof(s));
    ret = stat(pfile, &s);
    if(-1 == ret)
    {
        //发送错误的页面
            
        send_header(connfd, 404, "NOT FOUNT", "*.html", -1);
        send_file(connfd, "error.html");

        return -1;
    }
    if(S_ISREG(s.st_mode))
    {
        printf("\n------------cutting line----------\n");
        //发送响应报文的首部
        send_header(connfd, 200, "OK", get_mime_type(pfile), s.st_size);
        //发送文件内容
        send_file(connfd, pfile);
    }
    else if(S_ISDIR(s.st_mode))
    {
        //发送目录的内容
        send_header(connfd, 200, "OK", "*.html", -1);

        send_file(connfd, "dir_header.html");

        send_dir(connfd, pfile);

        send_file(connfd, "dir_tail.html");
        
    }

    return 0;
}

int main()
{
    int listenfd, connfd, sockfd;
    int ret, flags;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    int nready, i;

    int epollfd;
    struct epoll_event ev, evs[MAX];
    

    chdir(getenv("HOME"));
    chdir("html");

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == listenfd)
    {
        perror("socket");
        goto err0;
    }
    
    i = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &i, sizeof(i));
    if (-1 == ret)
    {
        perror("setsockopt");
        goto err1;
    }
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (-1 == ret)
    {
        perror("bind");
        goto err1;
    }

    ret = listen(listenfd, 25);
    if (-1 == ret)
    {
        perror("listen");
        goto err1;
    }
    
    epollfd = epoll_create(MAX);
    if (-1 == epollfd)
    {
        perror("epoll_create");
        goto err1;
    }

    bzero(&ev, sizeof(ev));
    ev.data.fd = listenfd;
    ev.events  = EPOLLIN;

    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    if (-1 == ret)
    {
        perror("epoll_ctl");
        goto err2;
    }

    while(1)
    {
        bzero(&evs, sizeof(evs));
        nready = epoll_wait(epollfd, evs, MAX, 3000);
        if (-1 == nready)
        {
            perror("epoll_wait");
            break;
        }
        if (0 == nready)
        {
            printf("s3 seconds timeout...\n");
            continue;
        }

        for(i=0; i<nready; i++ )
        {
            if(evs[i].events & EPOLLIN)
            {
                if(listenfd == evs[i].data.fd)
                {
                    bzero(&cliaddr, sizeof(cliaddr));
                    clilen = sizeof(cliaddr);

                    connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
                    if (-1 == connfd)
                    {
                        perror("accept");
                        continue;
                    }

                    printf("客户端%s:%d连接到服务端。。。\n", inet_ntoa(cliaddr.sin_addr),
                           ntohs(cliaddr.sin_port));

                    flags = fcntl(connfd, F_GETFL);
                    flags |=O_NONBLOCK;
                    fcntl(connfd, F_SETFL, flags);

                    bzero(&ev, sizeof(ev));
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connfd;

                    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
                    if (-1 == ret)
                    {
                        perror("epoll_ctl");
                        close(connfd);
                        continue;
                    }
                }
                else
                {
                    http_request(evs[i].data.fd);
                }

            }
        }
    }


    close(listenfd);
    close(epollfd);

    return 0;
err2:
    close(epollfd);
err1:
    close(listenfd);
err0:
    return 1;

}
