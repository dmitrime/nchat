#include "cli.h"

WINDOW *text, *type, *line;

int
main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: nclient [server name or ip]\n");
    exit(EXIT_FAILURE);
  }

  start_client(argv[1]);

  return 0;
}

int make_connection(char *addr)
{
  int cfd;
  struct sockaddr_in me;

  /* ip */
  if (addr[0] >= '0' && addr[0] <= '9')
  {
    if (inet_pton(AF_INET, addr, &me.sin_addr) <= 0)
      errcntl("inet_pton");
  }
  else /* domain name */
  {
    struct hostent *ht;
    if ((ht = gethostbyname(addr)) == NULL)
      errcntl("bad hostname, cleaning up...  ");

    /*me.sin_addr = *(struct in_addr*)ht->h_addr;*/
    memcpy(&me.sin_addr, ht->h_addr, sizeof(struct in_addr));
  }


  if ((cfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    errcntl("socket");

  memset(&me, 0, sizeof me);
  me.sin_family = AF_INET;
  me.sin_port = htons(PORT);

  if (connect(cfd, (struct sockaddr *)&me, sizeof me) == -1)
    errcntl("connect");

  return cfd;
}

void start_client(char *addr)
{
  int cfd;
  fd_set rdset;
  char buf[MXLINE];

  char name[64];

  if (setup_ncurses(&text, &type, &line) == -1)
    errcntl("ncurses");

  /* catch ctrl-c and ignore */
  signal(SIGINT, SIG_IGN);

  /* zero teh set */
  FD_ZERO(&rdset);


  waddstr(text, "Connecting...\n");
  wrefresh(text);

  /* do socket, inet_pton, connect, return the decriptor */
  cfd = make_connection(addr);

  /*----------------------------*/
  waddstr(text, "Connected to ");
  waddstr(text, addr);
  waddch (text, '\n');
  wrefresh(text);
  /*----------------------------*/

  /* get the nickname */
  do
  {
    waddstr(type, "(Enter your name): ");
    wgetnstr(type, name, 64 - 1);
  } while (!isalnum(name[0]));

  send_nick(cfd, name);

  for (;;)
  {
    /* set the descriptor set */
    FD_SET(fileno(stdin), &rdset);
    FD_SET(cfd, &rdset);

    /* set the promt */
    wclear(type);
    waddstr(type, ": ");
    wrefresh(type);
    /*---------------*/

    /* block in select */
    if (select(cfd + 1, &rdset, NULL, NULL, NULL) == -1)
      errcntl("select");

    /* check cfd */
    if (FD_ISSET(cfd, &rdset))
    {
      int r = 0;

      if ((r = read_input(cfd, buf)) != 0)
      {
	waddnstr(text, buf, r);
	wrefresh(text);
      }
      else /* end connection */
      {
	end_ncurses(text, type, line);
	close_connection(cfd);
	return;
      }
    }
    /* check stdin */
    if (FD_ISSET(fileno(stdin), &rdset))
    {
      /* ready to read stdin */
      if (wgetnstr(type, buf, MXLINE - 2) != ERR)
      {
	/* TODO check CMD */
	if (not_empty(buf)) /* not empty line */
	{
	  add_nl(buf);
	  write_output(cfd, buf, strlen(buf) + 1);
	}
      }
      else /* end connection */
      {
	end_ncurses(text, type, line);
	close_connection(cfd);
	return;
      }
    }
  }
}

int write_output(int fd, char *buf, unsigned len)
{
  int r;

  if (write(fd, buf, len) == -1)
    errcntl("write");

  return r;
}

int read_input(int fd, char *buf)
{
  int r;

  memset(buf, 0, MXLINE);
  if ((r = read(fd, buf, MXLINE)) == -1)
    errcntl("read");

  return r;
}

void close_connection(int fd)
{
  fprintf(stderr, "Connection terminated\n");
  close(fd);
  return;
}

void errcntl(char *x)
{
  end_ncurses(text, type, line);
  perror(x);
  exit(EXIT_FAILURE);
}

int setup_ncurses(WINDOW **text, WINDOW **type, WINDOW **line)
{
  int i;

  initscr();
  nl(); /* does not apper to work ... add \n manually*/

  if (has_colors() == TRUE)
    if (start_color() == ERR)
    {
      endwin();
      fprintf(stderr, "Error, ending\n");
      return -1;
    }

  *text = newwin(LINES - 2, COLS, 0, 0);
  *type = newwin(1, COLS, LINES - 1, 0);
  *line = newwin(1, COLS, LINES - 2, 0);

  if (*text == NULL || *type == NULL || *line == NULL)
  {
    endwin();
    fprintf(stderr, "Error, ending\n");
    return -1;
  }

  curs_set(0); /* no cursor */

  scrollok(*text, TRUE);
  scrollok(*type, TRUE);

  init_pair(1, COLOR_BLUE, COLOR_YELLOW);
  init_pair(2, COLOR_MAGENTA, COLOR_BLACK);

  wattron(*type, A_BOLD);
  wattron(*text, A_BOLD | COLOR_PAIR(2));
  wattron(*line, COLOR_PAIR(1));

  for (i = 0; i < COLS; i++)
    mvwaddch(*line, 0, i, '-');
  wrefresh(*line);

  return 0;
}

void end_ncurses(WINDOW *text, WINDOW *type, WINDOW *line)
{
  delwin(line);
  delwin(text);
  delwin(type);
  endwin();
}


void add_nl(char *buf)
{
  char *p = NULL;
  if (buf != NULL)
    if ((p = strchr(buf, '\0')) != NULL)
    {
      *p++ = '\n';
      *p = '\0';
    }
}

void send_nick(int d, char *n)
{
  char ji[100] = "/join ";
  strncat(ji, n, strlen(n) + 1);

  write_output(d, ji, strlen(ji) + 1);

  return;
}

int not_empty(char *s)
{
  if (s != NULL)
  {
    char *p = s;
    while (isspace(*p) && *p != '\0')
      p++;

    if (*p != '\0')
      return 1;
  }
  return 0;
}

