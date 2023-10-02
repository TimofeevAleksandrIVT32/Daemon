CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11

.PHONY: all clean

all: server client

server: main_server.c server.h server.c
	$(CC) $(CFLAGS) $^ -o $@

client: main_client.c client.h client.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f server client
	rm -f *.o
