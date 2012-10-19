NChat - a simple unix chat client and server
=====

nchat, nserver and nclient, -- simple unix chat client and server, based on the select() call and a linked list.

to build nserver: make nserver

to build nclient: make nclient

    ./nserver -- to start the server
    ./nclient [IP or hostname] -- to start the client

inside nclient:

    /quit or /qq - quit
    /list        - list all users
    /nick [nick] - change name
    /me          - speak in third person about yourself

Historical note
=====
This is a relly ancient project from times long ago when I was trying to learn unix network programming. 
It also happens to be my first big programming project of around 20KB of source code!
According to the file timestamps it was created in August 2005 and modified later in October.
I was proud of it at the time, but I don't think anyone has ever used it...
Wow! Where does the time fly?

