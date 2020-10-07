CFLAGS=-Iinclude -Llib
LIB=-lrequest

server: ./src/server.c
	gcc -o ./objects/server $(CFLAGS) ./src/server.c $(LIB)
clean: 
	rm -rf ./objects/server
