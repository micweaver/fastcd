/**
 * @author  lizhonghua@360.cn
 * @desc     提供网络服务，接收目录查询请求
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>

#include "fastcd.h"
#include "data.h"
#include "utils.h"

#define MAXPATHLEN 200

char cur_dir[MAXPATHLEN];

list_node *fail_dir;

extern int sum_dir;

#define MODEL_ALL 1  //搜索全部目录
#define MODEL_PWD 2 //搜索当前目录以下的目录
#define MODEL_SMART 3  //先搜索当前目录以下目录，结果为空再搜索全部目录

static int search_model = MODEL_SMART;	//搜索模式

int traverse_dir()
{

	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp, *tdp;
	char *ptr;

	if (lstat(cur_dir, &statbuf) < 0)
	{
		fprintf(stderr, "directory '%s' read fail,may not exist\n", cur_dir);
		return -1;
	}

	if (S_ISDIR(statbuf.st_mode) == 0)
	{
		return 0;
	}

	ptr = cur_dir + strlen(cur_dir);
	*ptr++ = '/';
	*ptr = 0;

	if ((dp = opendir(cur_dir)) == NULL)
	{
		list_node *failtmp = get_list_node(cur_dir);
		failtmp->dirname = my_strcat(failtmp->dirname, "  [no power]");
		add_list_node(&fail_dir, failtmp);
		return -1;
	}

	while ((dirp = readdir(dp)) != NULL)
	{

		if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;

		strcpy(ptr, dirp->d_name);

		if (lstat(cur_dir, &statbuf) < 0)
		{
			fprintf(stderr, "stat %s read fail\n", cur_dir);
			return -1;
		}
		if (S_ISDIR(statbuf.st_mode) == 0)
		{
			continue;
		}

		if ((tdp = opendir(cur_dir)) == NULL)
		{
			list_node *failtmp = get_list_node(cur_dir);
			failtmp->dirname = my_strcat(failtmp->dirname, "  [no power]");
			add_list_node(&fail_dir, failtmp);
			continue;
		}

		if (add_dir(dirp->d_name, cur_dir) < 0)
		{
			list_node *failtmp = get_list_node(cur_dir);
			failtmp->dirname = my_strcat(failtmp->dirname, "  [illegal char]");
			add_list_node(&fail_dir, failtmp);
		} else
		{
			printf("%s\n", cur_dir);
		}
		if (closedir(tdp) < 0)
		{
        		fprintf(stderr, "close %s fail\n", cur_dir);
        		return 0;
		}

		traverse_dir();
	}

	if (closedir(dp) < 0)
	{
		fprintf(stderr, "close %s fail\n", cur_dir);
		return 0;
	}

}

int init_server(struct sockaddr *addr)
{

	int fd;
	int err = 0;

	int reuse = 1;

	if ((fd = socket(addr->sa_family, SOCK_STREAM, 0)) < 0)
		return (-1);

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
	{
		err = errno;
		goto errout;
	}
	if (bind(fd, addr, sizeof(struct sockaddr)) < 0)
	{
		err = errno;
		goto errout;
	}

	if (listen(fd, BACKLOG) < 0)
	{
		err = errno;
		goto errout;
	}
	return (fd);

  errout:
	close(fd);
	errno = err;
	return (-1);
}

void init_sockaddr(struct sockaddr_in *name, const char *hostname,
				   uint16_t port)
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

int search_pre(char *input, char *output)
{

	fast_cd_net_req *param = (fast_cd_net_req *) input;
	list_node **res;

	fast_cd_net_res *netres = (fast_cd_net_res *) output;
	netres->errorno = 0;

	int i;
	init_data_store();
	if (param->cmd == CMD_QUERY)//目前只支持查询
	{							

		if (strlen(param->dirpre) <= 0 || strlen(param->pwddir) <= 0)
		{
			sprintf(netres->res, "param error");
			netres->datalen = strlen(netres->res);
			netres->errorno = 1;
			return -1;
		}

		int res_len = 0;

		char tpwd[PWDDIRLEN];
		strcpy(tpwd, param->pwddir);
		if (search_model == MODEL_ALL)
		{
			tpwd[0] = '/';
			tpwd[1] = 0;
		}
		res = search_dir(param->dirpre, tpwd, &res_len);
		if (res_len == 0 && !(tpwd[0] == '/' && tpwd[1] == 0)
			&& search_model == MODEL_SMART)
		{
			tpwd[0] = '/';
			tpwd[1] = 0;
			res = search_dir(param->dirpre, tpwd, &res_len);
		}

		if (res_len == 0)
		{

			sprintf(netres->res, "%s directory prefix not exist",param->dirpre);
			netres->datalen = strlen(netres->res);
			netres->errorno = 1;
			return -1;
		}

		int cur_len = 0;

		if (res_len > MAXRESNUM)
		{
			sprintf(netres->res,"the result is too many,supply the longer directory prefix ");
			netres->datalen = strlen(netres->res);
			netres->errorno = 1;
			return -1;
		}

		/*for(i=0 ;i < res_len; i++){
		   printf("%s\n",res[i]->dirname);
		   } */

		for (i = 0; i < res_len; i++)
		{
			strcpy(netres->res + cur_len, res[i]->dirname);
			cur_len += (strlen(res[i]->dirname) + 1);
		}
		netres->datalen = cur_len;

	} else
	{

		sprintf(netres->res, "unsupport cmd");
		netres->datalen = strlen(netres->res);
		netres->errorno = 1;
		return -1;
	}

}
void serve()
{

	int sockfd;
	int clfd;
	char sendbuf[RESDATALEN];
	char recvbuf[REQDATALEN];
	int byte;
	int cbyte;
	struct sockaddr_in name;
	bzero(&name, sizeof(name));
	init_sockaddr(&name, HOST, PORT);

	if ((sockfd = init_server((struct sockaddr *) &name)) < 0)
	{
		fprintf(stderr, "init server fail:%s\n", strerror(errno));
		exit(-1);
	}

	for (;;)
	{
		clfd = accept(sockfd, NULL, NULL);
		if (clfd < 0)
		{
			fprintf(stderr, " accept error: %s", strerror(errno));
			continue;
		}

		cbyte = 0;
		while ((byte =recv(clfd, recvbuf + cbyte, sizeof(fast_cd_net_req) - cbyte,0)) > 0)
		{
			cbyte += byte;
			if (byte == sizeof(fast_cd_net_req))
				break;
		}

		fast_cd_net_req *pt = (fast_cd_net_req *) recvbuf;
		//  printf("param:%d %s %s\n",pt->cmd,pt->dirpre,pt->pwddir);

		size_t len = strlen(pt->dirpre);
		if (pt->dirpre[len - 1] == '/')
		{
			pt->dirpre[len - 1] = 0;
		}
		search_pre(recvbuf, sendbuf);
		cbyte = 0;
		while ((byte =send(clfd, sendbuf + cbyte, sizeof(fast_cd_net_res) - cbyte,0)) > 0)
		{
			//  printf("byte:%d ressize:%d reqsize:%d\n",byte,sizeof(fast_cd_net_res),sizeof(fast_cd_net_req));
			cbyte += byte;
		}

		if (byte < 0)
		{
			fprintf(stderr, " send error: %s", strerror(errno));
		}

		close(clfd);
	}
}

void daemonize(const char *cmd)
{

	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;

	umask(0);

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		fprintf(stderr, "%s: can't get file limit\n", cmd);

	if ((pid = fork()) < 0)
		fprintf(stderr, "%s: can't fork\n", cmd);
	else if (pid != 0)
		exit(0);
	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		fprintf(stderr, "%s: can't ignore SIGHUP\n");
	if ((pid = fork()) < 0)
		fprintf(stderr, "%s: can't fork\n", cmd);
	else if (pid != 0)
		exit(0);

	if (chdir("/") < 0)
		fprintf(stderr, "%s: can't change directory to /\n");

	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

}

int main(int argc, char *argv[])
{

	int oc;
	int model;
	while ((oc = getopt(argc, argv, "m:")) != -1)
	{
		switch (oc)
		{
		case 'm':
			model = atoi(optarg);
			if (model == 1 || model == 2)
			{
				search_model = model;
			}
			break;
		}
	}

	if (argc == 1)
	{
		getcwd(cur_dir, MAXPATHLEN);
		traverse_dir();
	} else
	{

		int i;
		for (i = 1; i < argc; i++)
		{
			if (argv[i][0] == '.' && argv[i][1] == 0)
			{
				getcwd(cur_dir, MAXPATHLEN);
				traverse_dir();
			} else
			{
				if (argv[i][0] == '/')
				{
					strcpy(cur_dir, argv[i]);
				} else
				{
					getcwd(cur_dir, MAXPATHLEN);
					strcat(cur_dir, "/");
					strcat(cur_dir, argv[i]);
				}

				size_t len = strlen(cur_dir);
				if (cur_dir[len - 1] == '/')
				{
					cur_dir[len - 1] = 0;
				}
				traverse_dir();
			}

		}

	}

	list_node *cur_fail;
	if (fail_dir != 0)
	{
		fprintf(stderr, "\n\n[fail index directory  list:]\n");
		for (cur_fail = fail_dir; cur_fail != NULL; cur_fail = cur_fail->next)
		{
			printf("%s\n", cur_fail->dirname);
		}
	}

	printf("\n\n[sum of index files:]\n%d\n", sum_dir);

	daemonize("fastcd-server");
	serve();

	return 0;
}

/* --- unit test--------
   typedef struct _test_dir {
   char dir[100];
   char pdir[100];
   } test_dir;

   test_dir td[] = {
   {"ab","/ha"},
   {"ac","/ha"},
   {"abc","/ha"},
   {"abcd","/ha"},
   {"aecd","/ha"},
   {"aaaa","/ha"},
   {"bcd","/home/lizhonghua"},
   {"efg","/home/test/sub"},
   {0,0},
   };

 */
