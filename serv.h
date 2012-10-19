#ifndef S_UCS_HDR
#define S_UCS_HDR

#include "sysfunc.h"
#include <time.h>

#define MXMSGLEN 8192
#define MXLEN    64
#define PORT     4747

typedef struct clinfo
{
  char ip[16];
  char nick[MXLEN];
  int fd;
} Clinfo;

typedef struct clnode
{
  Clinfo clinfo;
  struct clnode *next;
} Clnode;

typedef struct list
{
  unsigned size;
  Clnode *start;
} Clist;

enum Msg_type
{
  AVERAGE = 1, JOIN, NICK, LIST, SELF, QUIT
};


void errcntl(char *er);
void start_chat(void);

int  build_msg(char *dest, const char *src, unsigned srclen, Clnode *c, enum Msg_type what);
void send_msg(char *msg, int len, Clist *clis);
void send_list(Clist *all, Clnode *one);

void add_client(Clist *clis, struct sockaddr_in nw, int newfd);
void remove_client(Clist *clis, Clnode *rem, fd_set *aset);

int find_new_maxfd(Clist *list);
Clnode *find_next_read(Clist *list, fd_set *set);

enum Msg_type check_cmd(char *s);
char *get_self(char *old);

int get_nick(char *to, char *from);
void set_nick(char *where, char *name);
int new_guest(Clnode *n);

int read_from(Clnode *n, char *msg);

#endif

