#include "utils.h"

char *my_strcat(char *dst, char *src)
{
	char *newstr = (char *) malloc(strlen(dst) + strlen(src) + 1);
	strcpy(newstr, dst);
	strcat(newstr, src);
	free(dst);
	return newstr;
}


