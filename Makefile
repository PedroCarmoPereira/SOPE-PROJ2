CFLAGS	=	-D_HPUX_SOURCE	-D_REENTRANT	-Wall	-Wextra
OBJECTS1	=	server.o log.o hashsum.o
OBJECTS2	=	user.o  log.o

all:	user	server

server: $(OBJECTS1)
	gcc $(OBJECTS1)	-pthread -o server

user: $(OBJECTS2)
	gcc $(OBJECTS2) -o user


server.o:	sope.h	constants.h	types.h	requestQueue.h hashsum.h
user.o:	sope.h	constants.h	types.h
queue.o: types.h requestQueue.h
log.o:	sope.h
hashsum.o: hashsum.h constants.h types.h

clean:
	rm *.o server user *.txt
