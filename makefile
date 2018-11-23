CC = gcc
CFLAGS = -O3  -Wall -D_REENTRANT

all: casanova put_client get_client  locking.o hash.o

casanova: casanova.c casanova.h locking.o hash.o
	$(CC) $(CFLAGS) -o casanova casanova.c locking.o hash.o  -I /usr/include/glib-2.0 $(shell pkg-config --cflags --libs glib-2.0) -lpthread  -lcrypto

hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -c hash.c  

locking.o: locking.c locking.h
	$(CC) $(CFLAGS) -c locking.c  

put_client: put_client.c casanova.h
	$(CC) $(CFLAGS) -o put_client put_client.c

get_client: get_client.c casanova.h
	$(CC) $(CFLAGS) -o get_client get_client.c

run:
	time ./put_client &
	time ./get_client &
	time ./casanova
	

clean: 
	rm -f *.o *.*~ *~ put_client get_client casanova 
	rm -f *.o *.*~ *~ telefones 
       
