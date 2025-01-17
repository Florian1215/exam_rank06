
PORT	= 8015
SRCS	= mini_serv.c
SERVER	= server

all:
		gcc -Wall -Werror -Wextra $(SRCS) -o $(SERVER)
		./$(SERVER) $(PORT)

clean:
		rm -rf $(SERVER)

nc:
		nc 127.0.0.1 $(PORT)
