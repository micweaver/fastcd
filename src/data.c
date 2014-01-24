/**
 * @author  lizhonghua@360.cn
 * @desc     对目录进行索引存储在内存中
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include <unistd.h>

#include <errno.h>

#include "data.h"
#include "utils.h"

char fchar[] ="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-";

trie_node trie_root;
list_node *res_list[MAXRESNUM];
int res_num;
static char *pwd_dir;
static int pwd_len;

void init_data_store()
{
	res_num = 0;
}

list_node *get_list_node(const char *dirname)
{

	if (strlen(dirname) <= 0)
	{
		return NULL;
	}

	list_node *new_node = (list_node *) malloc(sizeof(list_node));
	new_node->dirname = strdup(dirname);
	new_node->next = NULL;
	return new_node;

}

static trie_node *get_trie_node()
{
	trie_node *new_node = (trie_node *) malloc(sizeof(trie_node));
	new_node->completed = 0;
	new_node->dn = NULL;
	memset(new_node->next, 0, CHARNUM * sizeof(trie_node *));
	return new_node;

}

int add_list_node(list_node ** head, list_node * new_node)
{

	if (*head == NULL)
	{
		*head = new_node;
		return 0;
	}

	list_node *cur_pos = *head;
	while (cur_pos->next != NULL)
	{
		if (strcmp(new_node->dirname, cur_pos->dirname) == 0)
		{
			return -1;
		}
		cur_pos = cur_pos->next;
	}

	if (strcmp(new_node->dirname, cur_pos->dirname) == 0)
	{
		return -1;
	}
	cur_pos->next = new_node;

	return 0;

}

static int get_char_index(char c)
{

	int index;
	if (c >= 'a' && c <= 'z')
	{
		index = c - 'a';
	} else if (c >= 'A' && c <= 'Z')
	{
		index = c - 'Z' + 26;
	} else if (c >= '0' && c <= '9')
	{
		index = c - '0' + 52;
	} else if (c == '.')
	{
		index = 62;
	} else if (c == '_')
	{
		index = 63;
	} else if (c == '-')
	{
		index = 64;
	} else
	{
		return -1;
	}
	return index;

}

int add_dir(char *dir, char *full_dir)
{

	if (strlen(dir) <= 0 || strlen(full_dir) <= 0)
	{
		fprintf(stderr, "empty dir param\n");
		return -1;
	}

	trie_node *cur_node = &trie_root;
	trie_node *new_node;
	int index;
	while (*dir)
	{
		index = get_char_index(*dir);
		if (index < 0)
		{
			return -1;
		}
		if (cur_node->next[index] == 0)
		{
			new_node = get_trie_node();
			cur_node->next[index] = new_node;
		}
		cur_node = cur_node->next[index];
		dir++;
	}
	cur_node->completed = 1;
	list_node *new_list_node = get_list_node(full_dir);
	if (add_list_node(&(cur_node->dn), new_list_node) >= 0)
		sum_dir++;

	return 0;
}

static void get_all_dir_list(trie_node * tn)
{

	int i = 0;

	if (tn == NULL)
		return;

	list_node *cur_node;

	if (tn->completed && tn->dn)
	{
		cur_node = tn->dn;
		while (cur_node != NULL)
		{
			if (!strncmp(pwd_dir, cur_node->dirname, pwd_len))
			{
				if (res_num < MAXRESNUM)
				{
					res_list[res_num] = cur_node;
				}
				res_num++;
			}
			cur_node = cur_node->next;
		}
	}

	for (i = 0; i < CHARNUM; i++)
	{
		get_all_dir_list(tn->next[i]);
	}

}

list_node **search_dir(char *dirpre, char *curdir, int *res_len)
{

	if (strlen(dirpre) <= 0 || strlen(curdir) <= 0)
	{
		return NULL;
	}

	pwd_dir = curdir;
	pwd_len = strlen(curdir);

	*res_len = 0;

	trie_node *cur_node = &trie_root;
	int index;
	while (*dirpre)
	{
		index = get_char_index(*dirpre);
		if (index < 0)
		{
			return NULL;
		}
		if (cur_node->next[index] == NULL)
		{
			return NULL;
		}
		cur_node = cur_node->next[index];
		dirpre++;
	}

	get_all_dir_list(cur_node);
	*res_len = res_num;
	return res_list;

}
