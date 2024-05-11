BIN_FILES = servidor
CC = gcc
CFLAGS = -Wall -fPIC -g -I/usr/include/tirpc
CPPFLAGS = -I$(INSTALL_PATH)/include
LDFLAGS = -L$(INSTALL_PATH)/lib/
DEPS = claves_inter.h
OBJ = servidor.o  rpc_service_xdr.o rpc_service_clnt.o rpc_service_svc.o
LDLIBS = -lnsl -lpthread -ldl -ltirpc -lm

all: servidor

servidor: servidor.o rpc_service_svc.o rpc_service_xdr.o rpc_service_clnt.o
	$(CC) $(CFLAGS) -o servidor servidor.o $(LDFLAGS) $(LDLIBS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

servidor.o: servidor.c
	$(CC) $(CFLAGS) -c servidor.c

clean:
	rm -f $(BIN_FILES)

.PHONY: all clean
