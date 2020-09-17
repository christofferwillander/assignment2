CFLAGS=-Iinclude -Llib
LIB=-lrequest

src/main: src/main.c
	gcc -o src/main $(CFLAGS) src/main.c $(LIB)

server: src/server.c
	gcc -o src/server $(CFLAGS) src/server.c $(LIB)
