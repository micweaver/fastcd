#ifndef FASTCD_H
#define FASTCD_H

#define DIRPRELEN 50
#define PWDDIRLEN 200

#define MAX_RES_LEN 50000

#define CMD_ADD 1
#define CMD_QUERY 2

#define RES_DILIMITER '\0'

#define BACKLOG 10

#define HOST "localhost"
#define PORT 60819

#define  REQDATALEN  DIRPRELEN+PWDDIRLEN+8
#define  RESDATALEN  MAX_RES_LEN+16

typedef struct _fast_cd_net_req {
	int cmd;					// 1 ÃÌº”  2  ≤È—Ø
	char dirpre[DIRPRELEN];
	char pwddir[PWDDIRLEN];

} fast_cd_net_req;

typedef struct _fast_cd_net_res {

	int errorno;
	int datalen;
	char res[MAX_RES_LEN];

} fast_cd_net_res;

#endif
