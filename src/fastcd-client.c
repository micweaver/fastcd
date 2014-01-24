/**
 * @author  lizhonghua@360.cn
 * @desc     查询目录索引服务器客户端
 */

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>

#include "fastcd.h"

#define SENDBUFLEN 500
#define RECVBUFLEN 50000

#define MAXPATHLEN 200

#define MAXRESNUM 1000

void init_sockaddr(struct sockaddr_in *name, const char *hostname, uint16_t port)
{
	struct hostent *hostinfo;

	name->sin_family = AF_INET;
	name->sin_port = htons(port);
	hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL)
	{
		fprintf(stderr, "Unknown host %s.\n", hostname);
		exit(-1);
	}
	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}

static int is_num(char *s)
{

	while (*s)
	{
		if (!isdigit(*s))
			return 0;
		s++;
	}
	return 1;
}

int main(int argc, char *argv[])
{

	struct sockaddr_in name;
	int clfd;

	if (argc < 2)
	{
		fprintf(stderr, "usage: client directoyprefix [num]|[path]\n");
		exit(-1);
	}

	init_sockaddr(&name, HOST, PORT);
	clfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(clfd, (struct sockaddr *) &name, sizeof(name)) != 0)
	{
		fprintf(stderr,"connect %s:%d fail,check the fastcd-server is running\n",HOST, PORT);
		exit(-1);
	}

	fast_cd_net_req req;
	bzero(&req, sizeof(req));
	req.cmd = CMD_QUERY;

	strcpy(req.dirpre, argv[1]);

	if (argc > 2)
	{
		if (argv[2][0] == '/')
		{
			strcpy(req.pwddir, argv[2]);
		} else if (!is_num(argv[2]))
		{
			getcwd(req.pwddir, MAXPATHLEN);
			strcat(req.pwddir, "/");
			strcat(req.pwddir, argv[2]);
		} else
		{
			getcwd(req.pwddir, PWDDIRLEN);
		}
	} else
	{
		getcwd(req.pwddir, PWDDIRLEN);
	}

	// printf("param:%d %s %s\n",req.cmd,req.dirpre,req.pwddir);

	int byte = 0;
	int cbyte = 0;

	while ((byte = send(clfd, &req + cbyte, sizeof(req) - cbyte, 0)) > 0)
	{
		cbyte += byte;
	}

	char recvbuf[RESDATALEN];

	cbyte = 0;
	while ((byte =recv(clfd, recvbuf + cbyte, sizeof(fast_cd_net_res) - cbyte,0)) > 0)
	{
		cbyte += byte;
	}

	fast_cd_net_res *net_res = (fast_cd_net_res *) recvbuf;

	// printf("res:%d %d %s\n",net_res->errorno,net_res->datalen,net_res->res);
	if (net_res->errorno != 0)
	{
		net_res->res[net_res->datalen] = 0;
		fprintf(stderr, "%s\n", net_res->res);
		exit(-1);
	}

	char *res_list[MAXRESNUM];

	int i;
	int res_cnt = 0;
	res_list[res_cnt] = net_res->res;

	int is_legal = 0;
	for (i = 0; i < net_res->datalen; i++)
	{
		if (net_res->res[i] == 0)
		{
			is_legal = 1;
			if (res_cnt == 0 && i == net_res->datalen - 1)
			{
				printf("%s\n", res_list[0]);
				exit(1);
			}
			if (i < net_res->datalen - 1)
			{
				res_list[++res_cnt] = net_res->res + i + 1;
				is_legal = 0;
			}
		}

	}

	if (!is_legal)
	{
		fprintf(stderr, "the result data is bad\n");
		exit(-1);
	}

	int num;
	if (argc > 2 && is_num(argv[2]))
	{
		num = atoi(argv[2]);
		if (num > res_cnt + 1)
		{
			fprintf(stderr, "the num is too large,max num:%s\n", res_cnt + 1);
			exit(-1);
		}
		printf("%s\n", res_list[num - 1]);
		exit(1);

	} else
	{
		for (i = 0; i <= res_cnt; i++)
		{
			printf("%s\n", res_list[i]);
		}
		exit(2);
		if (num > res_cnt + 1)
		{
			fprintf(stderr, "the num is too large,max num:%d\n", res_cnt + 1);
			exit(-1);
		}
	}

	printf("some bad thing happen\n");
	exit(-1);

}
