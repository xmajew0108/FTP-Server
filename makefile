CC      = gcc
CFLAGS  = -Wall -g
CFLAGS_DEBUG = -Wall -g -DDEBUG

all: cliente servidor

cliente: cliente.c header.h structLog.h
	$(CC) $(CFLAGS) -o cliente cliente.c -lssl -lcrypto

cliente_deb:
	$(CC) $(CFLAGS) -o cliente cliente.c -lssl -lcrypto

servidor: servidor.c utilitats.c utilitats.h header.h structLog.h
	$(CC) $(CFLAGS_DEBUG) -o servidor servidor.c utilitats.c

clean:
	rm -f cliente servidor