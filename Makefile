CC = gcc
CFLAGS = -Wall

all: servidor

servidor: servidor.o
	$(CC) $(CFLAGS) -o servidor servidor.o -L. -pthread 

servidor.o: servidor.c
	$(CC) $(CFLAGS) -c servidor.c

send-recv.o: send-recv.c
	$(CC) $(CFLAGS) -fPIC -c send-recv.c -o send-recv.o

clean:
	rm -f cliente servidor *.o
