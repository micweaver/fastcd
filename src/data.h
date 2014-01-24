#ifndef DATA_H
#define DATA_H

#define CHARNUM  65
#define MAXRESNUM 100

typedef struct _list_node list_node;

struct _list_node {
	char *dirname;
	list_node *next;
};

typedef struct _trie_node trie_node;

struct _trie_node {
	int completed;
	trie_node *next[CHARNUM];
	list_node *dn;
};

int sum_dir;

void init_data_store();
int add_dir(char *dir, char *full_dir);
list_node **search_dir(char *dirpre, char *curdir, int *res_len);
list_node *get_list_node(const char *dirname);
int add_list_node(list_node ** head, list_node * new_node);

#endif
