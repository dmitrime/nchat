#include "serv.h"

int
main(void)
{
  start_chat();
  return 0;
}

void start_chat(void)
{
  int lfd, cfd, maxfd;
  char rdmsg[MXMSGLEN];
  char wrmsg[MXMSGLEN];
  struct sockaddr_in server, client;
  int optval = 1;
  socklen_t clen;
  fd_set rdset, aset;
  Clist clis;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    errcntl("socket");

  memset(&server, 0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1)
    errcntl("setsockopt (SO_REUSEADDR)");

  if (bind(lfd, (struct sockaddr*)&server, sizeof server) == -1)
    errcntl("bind");

  if (listen(lfd, 1024) == -1)
    errcntl("listen");

  FD_ZERO(&aset);
  FD_SET(lfd, &aset);
  maxfd = lfd;
  clis.size = 0;
  clis.start = NULL;

  for (;;) /* main loop */
  {
    int ready = 0;
    rdset = aset;
    if ((ready = select(maxfd + 1, &rdset, NULL, NULL, NULL) == -1)) /* TODO EINTR */
      errcntl("select");

    if (FD_ISSET(lfd, &rdset)) /* have a new connection */
    {
      clen = sizeof client;
      if ((cfd = accept(lfd, (struct sockaddr *)&client, &clen)) == -1)
	errcntl("accept");

      add_client(&clis, client, cfd);
      /* TODO log */

      if (cfd > maxfd)
	maxfd = cfd;

      FD_SET(cfd, &aset); /* set new decriptor */

      if (--ready <= 0)
	continue;
    }

    while (ready >= 0) /* find from whom and read a msg */
    {
      Clnode *nc = NULL;
      int rd;
      char new_name[MXLEN];

      nc = find_next_read(&clis, &rdset);
      if (nc == NULL)
	break;

      if ((rd = read_from(nc, rdmsg)) == 0) /* remove and close it */
      {
	remove_client(&clis, nc, &aset);
	maxfd = find_new_maxfd(&clis);  /* we may need to search for new high descriptor */
	if (maxfd == -1) /* no more clients, set maxfd to listen descriptor */
	  maxfd = lfd;
      }
      else
      {
	switch (check_cmd(rdmsg))
	{
	  case AVERAGE: /* average message */
	    rd = build_msg(wrmsg, rdmsg, rd, nc, AVERAGE);
	    break;

	  case QUIT: /* /q or /quit command */
	    rd = build_msg(wrmsg, NULL, 0, nc, QUIT);
	    remove_client(&clis, nc, &aset);
	    break;

	  case NICK: /* /nick command */
	    if (get_nick(new_name, rdmsg))
	    {
	      rd = build_msg(wrmsg, new_name, strlen(new_name), nc, NICK);
	      set_nick(nc->clinfo.nick, new_name);
	    }
	    break;

	  case JOIN: /* /join name, if guest is new*/
	    if (new_guest(nc))
	    {
	      get_nick(new_name, rdmsg);
	      set_nick(nc->clinfo.nick, new_name);
	      /*send_motd(nc); TODO */
	      rd = build_msg(wrmsg, NULL, 0, nc, JOIN);
	    }
	    else rd = -1;
	    break;

	  case LIST:
	    send_list(&clis, nc);  /*send a list of clients */
	    rd = -1;        /* don't do anything after */
	    break;

	  case SELF:
	    {
	      char *p;
	      if ((p = get_self(rdmsg)) != NULL)
		rd = build_msg(wrmsg, p, (rd - (p - rdmsg)), nc, SELF);
	      break;
	    }

	  default: /* bad string, skip it */
	    rd = -1;
	    break;
	}

      }
      if (rd != -1)
	send_msg(wrmsg, rd, &clis);
    }
    ready--;
  }

  return;
}

/* builds a message using the enum Msg_type to build different message formats */
int build_msg(char *dest, const char *src, unsigned srclen, Clnode *c, enum Msg_type what)
{
  if (c == NULL)
    return -1;

  int nicklen = strlen(c->clinfo.nick);
  int iplen = strlen(c->clinfo.ip);
  int totl = 0;

  char vr[9];
  struct tm *kl;
  time_t now;

  memset(dest, 0, MXMSGLEN);
  memset(vr, 0, 9);

  /* time in [23:59] format begin */
  now = time(NULL);
  if ((kl = localtime(&now)) == NULL)
    errcntl("localtime");
  strftime(vr, 9, "[%H:%M] ", kl);


  if (what == JOIN)
  {
    totl = nicklen + 29 + 9;

    if (totl >= MXMSGLEN)
      return -1;
    else
    {
      strncpy(dest, vr, 9);
      strncat(dest, c->clinfo.nick, nicklen);
      strncat(dest, " has joined the conversation\n", 29);
    }
  }
  else if (what == SELF)
  {
    totl = nicklen + srclen + 7 + 9;

    if (totl >= MXMSGLEN)
      return -1;
    else
    {
      strncpy(dest, vr, 9);
      strncat(dest, "** ", 3);
      strncat(dest, c->clinfo.nick, nicklen);
      strncat(dest, " is ", 4);
      strncat(dest, src, srclen);
    }
  }
  else if (what == NICK)
  {
    totl = nicklen + srclen + 21 + 9;

    if (totl >= MXMSGLEN)
      return -1;
    else
    {
      strncpy(dest, vr, 9);
      strncat(dest, "// ", 3);
      strncat(dest, c->clinfo.nick, nicklen);
      strncat(dest, " is now known as ", 17);
      strncat(dest, src, srclen);
      strncat(dest, "\n", 1);
    }
  }
  else if (what == QUIT)
  {
      totl = nicklen + iplen + 16 + 9; /* + time */

      /* FORMAT:[00:00] ###GUEST1 has quit (127.0.0.1)\n */
    if (totl >= MXMSGLEN) /* TODO log this */
      return -1;
    else
    {
      strncpy(dest, vr, 9);
      strncat(dest, "## ", 3);
      strncat(dest, c->clinfo.nick, nicklen);
      strncat(dest, " has quit ", 10);
      strncat(dest, "(", 1);
      strncat(dest, c->clinfo.ip, iplen);
      strncat(dest, ")\n", 2);
    }
  }
  else if (what == AVERAGE)
  {
    totl = nicklen + 2 + srclen + 9 + 1;

    /* FORMAT:[00:00] GUEST2: SRC_MSG */
    if (totl  >= MXMSGLEN) /* TODO log this */
      return -1;
    else
    {
      strncpy(dest, vr, 9);
      strncat(dest, c->clinfo.nick, nicklen);
      strncat(dest, ": ", 2);
      strncat(dest, src, srclen);
    }
  }
  return totl;
}

/* send msg to all clients */
void send_msg(char *msg, int len, Clist *clis)
{
  Clnode *n;
  if (clis == NULL || clis->start == NULL)
    return;
  n = clis->start;

  while (n != NULL)
  {
    if (write(n->clinfo.fd, msg, len) == -1)
      errcntl("write");
    n = n->next;
  }

  return;
}

void errcntl(char *er) /* TODO make it better */
{
  perror(er);
  exit(EXIT_FAILURE);
}

void remove_client(Clist *clis, Clnode *rem, fd_set *aset)
{
  Clnode *n;

  if (clis == NULL || clis->start == NULL || rem == NULL)
    return;

  clis->size--;

  n = clis->start;
  if (n == rem)
  {
    clis->start = n->next;
    FD_CLR(rem->clinfo.fd, aset);
    
    /*shutdown(n->clinfo.fd, SHUT_WR);*/
    close(n->clinfo.fd);

    free(n);
  }
  else
  {
    while (n->next != rem)
      n = n->next;

    n->next = rem->next;
    FD_CLR(rem->clinfo.fd, aset);

    /*shutdown(rem->clinfo.fd, SHUT_WR);*/
    close(rem->clinfo.fd);

    free(rem);
  }
}

int find_new_maxfd(Clist *list)
{
  Clnode *n;
  int mfd;

  if (list == NULL || list->start == NULL)
    return -1;
  n = list->start;
  mfd = n->clinfo.fd;

  while (n != NULL)
  {
    if (n->clinfo.fd > mfd)
      mfd = n->clinfo.fd;

    n = n->next;
  }
  return mfd;
}

int read_from(Clnode *n, char *msg)
{
  int r = 0;

  memset(msg, 0, MXMSGLEN);

  if ((r = read(n->clinfo.fd, msg, MXMSGLEN)) == -1)
    errcntl("read");

  return r;
}

Clnode *find_next_read(Clist *list, fd_set *set)
{
  Clnode *n;
  if (list == NULL || list->start == NULL)
    return NULL;

  n = list->start;

  while (n != NULL)
  {
    if (FD_ISSET(n->clinfo.fd, set))
    {
      FD_CLR(n->clinfo.fd, set); /* unset it */
      return n;
    }
    n = n->next;
  }

  return NULL;
}

void add_client(Clist *clis, struct sockaddr_in nw, int newfd)
{
  /* TODO nick */
  if (clis->size == 0) /* no nodes */
  {
    clis->start = malloc(sizeof (Clnode));
    if (clis->start == NULL)
      errcntl("malloc");

    clis->size++;
    clis->start->clinfo.fd = newfd;
    if (inet_ntop(AF_INET, &nw.sin_addr, clis->start->clinfo.ip, (socklen_t)16) == NULL)
      errcntl("inet_ntop");

    set_nick(clis->start->clinfo.nick, NULL);

    clis->start->next = NULL;
  }
  else /* add in front, so that it's easier to search for maxfd later */
  {
    Clnode *n = malloc(sizeof (Clnode));
    if (n == NULL)
    {
      /* TODO remove/free all OR log and return */
      errcntl("malloc");
    }

    clis->size++;
    n->clinfo.fd = newfd;
    if (inet_ntop(AF_INET, &nw.sin_addr, n->clinfo.ip, (socklen_t)16) == NULL)
      errcntl("inet_ntop");

    set_nick(n->clinfo.nick, NULL);

    n->next = clis->start;
    clis->start = n;
  }
}

/* Set the default nick (guestN)*/
void set_nick(char *where, char *name) 
{
  if (where == NULL)
    return;

  if (name == NULL)
  {
    char cat[MXLEN];
    const char *defname = "guest";
    static unsigned num = 1;
    int len;

    len = sprintf(cat, "%u", num);

    if (MXLEN - 1 > len + strlen(defname))
    {
      strcpy(where, defname);
      strcat(where, cat);
    }
    num++; /* increment count */
  }
  else
  {
    strncpy(where, name, MXLEN - 1);
  }
}

/* extract the name from /nick name (or /join name)*/
int get_nick(char *to, char *from)
{
  if (to != NULL && from != NULL)
  {
    char *p;

    p = strtok(from, " \t\n");
    if (p != NULL)
      p = strtok(NULL, " \t\n");
    if (p != NULL)
    {
      /*char *q = p;
      while (!isspace(*q) && *q != '\n' && *q != '\0')
	q++;

      if (q - p + 1 < MXLEN)*/
      if (strlen(p) + 1 < MXLEN)
      {
	strcpy(to, p);
	return 1;
      }
    }
  }
  return 0;
}

/* search for chat commands is 's'
 * commands are:
 * /nick name - change name, return NICK
 * /qq or /quit - quit, return QUIT
 * /list - list all clients connected, return LIST
 */
enum Msg_type check_cmd(char *s)
{
  if (s != NULL)
  {
    char *p = s;
    while (isspace(*p) && *p != '\0')
      p++;

    if (*p != '/')
      return AVERAGE;
    else
    {
      p++;

      if (!strncmp(p, "quit", 4) || !strncmp(p, "qq", 2))
	return QUIT;
      if (!strncmp(p, "list", 4))
	return LIST;
      if (!strncmp(p, "nick ", 5))
	return NICK;
      if (!strncmp(p, "join ", 5))
	return JOIN;
      if (!strncmp(p, "me ", 3))
	return SELF;
    }
    return AVERAGE;
  }
  return -1;
}

int new_guest(Clnode *n)
{
  if (n != NULL)
  {
    if (!strncmp(n->clinfo.nick, "guest", 5))
      return 1;
  }

  return 0;
}

char *get_self(char *old)
{
  if (old != NULL)
  {
    char *p = old;
    while (isspace(*p) && *p !='\0')
      p++;

    if (*p == '/')
    {
      p += 4; /* /me */
      return p;
    }
  }
  return NULL;
}

void send_list(Clist *all, Clnode *one)
{
  char li[MXMSGLEN];
  int len = 0, t;
  Clnode *cur = NULL;

  memset(&li, 0, sizeof li);

  if (all == NULL || one == NULL)
    return;

  cur = all->start;
  while (cur != NULL)
  {
    t = 0;
    t = strlen(cur->clinfo.nick); /* + '\n' + '\0' */

    if (len + t + 1 >= MXMSGLEN - 1)
      break;

    strncat(li, cur->clinfo.nick, t);
    strncat(li, "\n", 1);

    len = len + t + 1;

    cur = cur->next;
  }

  len++; /* '\0' */
  if (write(one->clinfo.fd, li, len) == -1)
    errcntl("write");

  return;
}

