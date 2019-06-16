#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "pub.h"

#define SIZE 1024
#define MAXSIZE 4096

int send_header(int connfd, int code, char* status, char* file_type, int file_length)
{

	int ret = -1;
	char buffer[MAXSIZE];

	bzero(buffer, MAXSIZE);
	sprintf(buffer, "HTTP1.1 %d %s\r\n", code, status);
	ret = send(connfd, buffer, strlen(buffer), 0);
	if(-1 == ret)
	{
		perror("send");
		goto err0;
	}

	bzero(buffer, MAXSIZE);
	sprintf(buffer, "Content-Type: %s\r\n", file_type);
	ret = send(connfd, buffer, strlen(buffer), 0);
	if(-1 == ret)
	{
		perror("send");
		goto err0;
	}

	bzero(buffer, MAXSIZE);
	sprintf(buffer, "Content-Length %d\r\n", file_length);
	ret = send(connfd, buffer, strlen(buffer), 0);
	if(-1 == ret)
	{
		perror("send");
		goto err0;
	}

	bzero(buffer, MAXSIZE);
	sprintf(buffer, "Connection: close\r\n" );
	ret = send(connfd, buffer, strlen(buffer), 0);
	if(-1 == ret)
	{
		perror("send");
		goto err0;
	}

	bzero(buffer, MAXSIZE);
	sprintf(buffer, "\r\n");
	ret = send(connfd, buffer, strlen(buffer), 0);
	if(-1 == ret)
	{
		perror("send");
		goto err0;
	}

	return 0;
err0:
	return 1;
}

int send_file(int connfd, char* file_name)
{
	int ret = -1;
	int fd = -1;
   
	char buffer[MAXSIZE];

	fd = open(file_name, O_RDONLY);
	if(-1 == fd)
	{
		perror("open");
	}
	while(1)
	{
		bzero(buffer, MAXSIZE);
		ret = read(fd, buffer, MAXSIZE );
		if(ret <=0)
		{
			perror("read");
			break;
		}
		ret = send(connfd, buffer, ret, 0 );
		if(ret <= 0)
		{
			perror("send");
			break;
		}
	}

	close(fd);

	return 0;
}

int send_dir(int connfd, char* file_name)
{
	int ret = -1;
	char buffer[MAXSIZE];
	
	struct dirent* d = NULL;
	DIR* dir = NULL;

	dir = opendir(file_name);
	if(NULL == dir)
	{
		perror("opendir");
		return 1;
	}

	while(1)
	{
		d = readdir(dir);
		if(NULL == d)
		{
			break;
		}

		bzero(buffer, MAXSIZE);
		if(d->d_type ==  DT_REG)
		{
			sprintf(buffer, "<li><a href = '%s'> %s</a></li>", d->d_name, d->d_name);
		}
		else if(d->d_type == DT_DIR)
		{
			sprintf(buffer, "<li><a href = '%s/'> %s</a></li>", d->d_name, d->d_name);
		}

		ret = send(connfd, buffer, strlen(buffer), 0);
		if(ret <= 0)
		{
			perror("send");
            return 1;
		}
	}

	closedir(dir);

	return 0;
}

int http_request(int connfd)
{
	int ret = -1;
	char buffer[MAXSIZE];
	char str[MAXSIZE];

	char method[32];
	char url[1024];
	char version[32];
	char status[32];

	struct stat s;
	char* pfile = NULL;

	bzero(buffer, MAXSIZE);
	ret = get_line(connfd, str, MAXSIZE);
	if(-1 == ret)
	{
		printf("get_line failed\n");
		return 1;
	}

    //打印http请求行
	printf("buffer: %s", str);

	sscanf(str, "%s %s %s\r\n", method, url, version);

	printf("\033[31mmethod:\033[0m %s, \033[31murl: \033[0m%s, \033[31mversion: \033[0m%s\n", method, url, version);

	strdecode(url, url);
	printf("\033[31mmethod:\033[0m %s, \033[31murl: \033[0m%s, \033[31mversion: \033[0m%s\n", method, url, version);

    //打印请求头
	while(1)
	{
		bzero(buffer, MAXSIZE);
		ret = get_line(connfd, buffer, MAXSIZE);
		if(ret <= 0)
		{
			perror("recv");
			break;
		}
		printf("%s", buffer);
	}

	if(strncmp(method, "GET", 3) != 0)
	{
		printf("This is not a GET req\n");
		return 1;
	}

	if(strcmp(url, "/") == 0)
	{
		pfile = "./";
	}
	else
	{
		pfile = url + 1 ;
	}

	bzero(&s, sizeof(s));
	ret = stat(pfile, &s);
	if(-1 == ret)
	{
		perror("stat");
		send_header(connfd, 404, "NOT FOUND", ".html", -1);
		send_file(connfd, "error.html");
		return 1;
	}

	if(S_ISREG(s.st_mode))
	{
		send_header(connfd, 200, "OK", get_mime_type(pfile), s.st_size);
		send_file(connfd, pfile);
	}
	else if(S_ISDIR(s.st_mode))
	{
		send_header(connfd, 200, "OK", "*.html", -1);
        send_file(connfd, "dir_header.html");
		send_dir(connfd, pfile);
        send_file(connfd, "dir_tail.html");
	}

	return 0;
}

int main(int argc, char* argv[])
{
    printf("-----line:%d func:%s---\n", __LINE__, __FUNCTION__);
	int ret = -1;
	int sockfd = -1;
	int connfd = -1;
	int epollfd = -1;
	int i = 1;

	int flag = 0;

	int count = 0;

	char ip_buf[16];

	struct sockaddr_in addr;
	struct sockaddr_in from;
	struct epoll_event e;
	struct epoll_event re[128];

	socklen_t len = 0;

	if(2 != argc)
	{
		printf("usage: server port ");
		goto err0;
	}

	ret = chdir(getenv("HOME"));
	if(-1 == ret)
	{
		perror("chdir");
		goto err0;
	}
	ret = chdir("html");
	if(-1 == ret)
	{
		perror("chdir");
		goto err0;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sockfd)
	{
		perror("socket");
		goto err0;
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &i, sizeof(i));
	if(-1 == ret)
	{
		perror("setsockopt");
		goto err1;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd, (void*)&addr, sizeof(addr));
	if(-1 == ret)
	{
		perror("bind");
		goto err1;
	}

	listen(sockfd, 10);

	epollfd = epoll_create(1024);
	if(-1 == epollfd)
	{
		perror("epoll_create");
		goto err1;
	}

	e.events = EPOLLIN;
	e.data.fd = sockfd;

	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &e);
	if(-1 == ret)
	{
		perror("epoll_ctl");
		goto err2;
	}

	i = 0;
	while(1)
	{
		bzero(re, sizeof(re));
		ret = epoll_wait(epollfd, re, 128, 300000);
		if(-1 == ret)
		{
			perror("epoll_wait");
			break;
		}//epoll_wait goes wrong!
		if(0 == ret)
		{
            continue;
		}//epoll timo
		else
		{
			count = ret;
			for(int i = 0; i<count; i++)
			{
				if(re[i].events & EPOLLIN)
				{
					if(re[i].data.fd == sockfd)
					{
						len = sizeof(from);
						bzero(&from, len);
						connfd = accept(sockfd, (void*)&from, &len);
						printf("\033[32mClient \033[0m%s:%d \033[32mconnected, fd = \033[0m%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), connfd);
						if(-1 == connfd)
						{
							perror("accept");
							continue;
						}
						flag = fcntl(connfd, F_GETFL);
						if(-1 == flag)
						{
							perror("fcntl");
							break;
						}
						flag |= O_NONBLOCK;
						fcntl(connfd, F_SETFL, flag);

						e.events = EPOLLIN | EPOLLET;
						e.data.fd = connfd;

						ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &e);
						if(-1 == ret)
						{
							perror("epoll_ctl");
							close(connfd);
							continue;
						}
					}
					else
					{
						http_request(re[i].data.fd);
					}
				}
			}
		}//else
	}//while(1)
    return 0;

err2:
	close(epollfd);
err1:
	close(sockfd);
err0:
	return 1;

}

