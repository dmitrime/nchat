#ifndef C_UCC_HDR
#define C_UCC_HDR

#include "sysfunc.h"
#include <ncurses.h>
#include <signal.h>
#include <netdb.h>


#define MXLINE 8192
#define PORT 4747

void start_client(char *addr);
void errcntl(char *x);
int  setup_ncurses(WINDOW **text, WINDOW **type, WINDOW **line);
void end_ncurses(WINDOW *text, WINDOW *type, WINDOW *line);
int  make_connection(char *addr);
int  write_output(int fd, char *buf, unsigned len);
int  read_input(int fd, char *buf);
void close_connection(int fd);
void add_nl(char *buf);
void send_nick(int d, char *n);
int  not_empty(char *s);

#endif

