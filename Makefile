CC       = gcc

OBJECTS1 = nchat.o
OBJECTS2 = face.o

HEADERS1 = sysfunc.h serv.h
HEADERS2 = sysfunc.h cli.h

FLAGS    = -Wall -ansi -O2
LINK     = -lncurses

PROG1    = nserver
PROG2    = nclient

$(PROG1): $(OBJECTS1) $(HEADERS1)
	   $(CC) $(OBJECTS1) $(FLAGS) -o $(PROG1)

$(PROG2): $(OBJECTS2) $(HEADERS2)
	   $(CC) $(OBJECTS2) $(FLAGS) -o $(PROG2) $(LINK)

clean:
	rm -fr $(PROG1) $(OBJECTS1) $(PROG2) $(OBJECTS2)


