#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#include "pub.h"
#include "./log.h"

#define _WORK_DIR_ "%s/html"
#define _DIR_PREFIX_FILE_ "html/dir_header.html"
#define _DIR_TAIL_FILE_   "html/dir_tail.html"

#define SIZE 128
#define PORT 10086


//组织相应报头消息
int copy_header(struct bufferevent* bev, int op, char* msg, char* filetype, int filesize)
{
    char buf[4096] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", op, msg);
    sprintf(buf, "%sContent-Type:%s\r\n", buf, filetype);
    if(filesize >= 0)
    {
        sprintf(buf, "%sContent-Length:%d\r\n", buf, filesize);
    }

    strcat(buf, "\r\n");

    bufferevent_write(bev, buf, strlen(buf));
    return 0;
}

//发送文件
int copy_file(struct bufferevent* bev, const char* strFile)
{
    int fd = open(strFile, O_RDONLY);
    char buf[1024] = {0};
    int ret;
    while( (ret = read(fd, buf, sizeof(buf))) > 0)
    {
        bufferevent_write(bev, buf, ret);
    }
    close(fd);
    return 0;
}

//发送目录
int send_dir(struct bufferevent* bev, const char* strPath)
{
    copy_file(bev, _DIR_PREFIX_FILE_);
    char bufline[1024] = {0};

    DIR* dir = opendir(strPath);
    struct dirent* d = NULL;

    while(d = readdir(dir))
    {
        struct stat sb;
        stat(d->d_name, &sb);
        if(d->d_type == DT_DIR)
        {
            memset(bufline, 0x00, sizeof(bufline));
            sprintf(bufline, "<li><a href='%s\'>%32s</a> %8ld</li>", d->d_name, d->d_name, sb.st_size);
            bufferevent_write(bev, bufline, strlen(bufline));
        }
        else if(d->d_type == DT_REG)
        {
            memset(bufline, 0x00, sizeof(bufline));
            sprintf(bufline, "<li><a href='%s'>%32s</a>  %8ld </li>", d->d_name, d->d_name, sb.st_size);
            bufferevent_write(bev, bufline, strlen(bufline));
        }
    }
    closedir(dir);
    copy_file(bev, _DIR_TAIL_FILE_);
}

int http_request(struct bufferevent* bev, char* path)
{
    //中文解码
    strdecode(path, path);
    char *strPath = path;
    if(strcmp(strPath, "/")==0 || strcmp(strPath, "/.")==0)
    {
        strPath = "./";
    }
    else
    {
        strPath = path + 1;
    }

    struct stat sb;

    if(stat(strPath, &sb) < 0)
    {
        copy_header(bev, 404, "NOT FOUND", get_mime_type("error.html"), -1);
        copy_file(bev, "error.html");
        return -1;
    }

    if(S_ISDIR(sb.st_mode))
    {
        copy_header(bev, 200, "OK", get_mime_type("*.html"), -1);
        send_dir(bev, strPath);
    }
    if(S_ISREG(sb.st_mode))
    {
        copy_header(bev, 200, "OK", get_mime_type(strPath), sb.st_size);
        copy_file(bev, strPath);
    }

    return 0;
}


//读回调
void read_cb(struct bufferevent* bev, void* arg)
{
    char buf[256] = {0};
    char method[10], path[256], protocol[10];
    int ret = bufferevent_read(bev, buf, sizeof(buf));
    if(ret > 0)
    {
        sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
        if(strncmp(method, "GET", 3) == 0)
        {
            //处理客户端的get请求
            char bufline[256];
            write(STDOUT_FILENO, buf, ret);

            while( (ret = bufferevent_read(bev, bufline, sizeof(bufline))) > 0 )
            {
                write(STDOUT_FILENO, bufline, ret);
                bzero(bufline, sizeof(bufline));
            }

            //解析请求数据
            http_request(bev, path);//处理请求
        }
    }
}

//事件回调
void bevent_cb(struct bufferevent* bev, short what, void* arg)
{
    if(what & BEV_EVENT_EOF)//客户端关闭
    {
        printf("client closed...\n");
        bufferevent_free(bev);
    }
    else if(what & BEV_EVENT_ERROR)
    {
        printf("err to client closed...\n");
        bufferevent_free(bev);
    }
    else if(what & BEV_EVENT_CONNECTED)//连接成功
    {
        printf("client connect ok...\n");
    }
}

void listen_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* addr, int socklen, void* arg)
{
    //定义与客户通信的 bufferevent
    struct event_base* base = (struct event_base*)arg;
    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, bevent_cb, base); //设置回调
    bufferevent_enable(bev, EV_READ|EV_WRITE); //启用读和写
}

int
main(int argc, char** argv)
{
    chdir(getenv("HOME"));
    chdir("html");

    struct sockaddr_in addr;
    struct event_base* base = NULL;
    struct evconnlistener* listener = NULL;

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    base = event_base_new();
    if(NULL == base)
    {
        printf("event_base_new failed...\n");
        goto err0;
    }

    listener = evconnlistener_new_bind(base, listen_cb, base, 
                                       LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
                                       -1, (struct sockaddr*)&addr, sizeof(addr));
    if(NULL == listener)
    {
        printf("evconnlistener_new_bind failed...\n");
        goto err1;
    }

    event_base_dispatch(base);
    
    event_base_free(base);
    evconnlistener_free(listener);

    return 0;

err1:
    event_base_free(base);
err0:
    return 1;
}
