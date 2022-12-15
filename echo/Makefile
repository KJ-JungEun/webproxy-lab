CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: echoserveri echoclient

echoserveri: echoserveri.c csapp.o
	$(CC) $(CFLAGS) -o echoserveri echoserveri.c csapp.o $(LIB)

echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)
	

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c


clean:
	rm -f *.o echo echoserveri echoclient *~
