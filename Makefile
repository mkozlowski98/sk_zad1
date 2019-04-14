TARGET: netstore-client netstore-server

CC = gcc
CFLAGS = -Wall -O2

netstore-client: klient.c
	$(CC) $(CFLAGS) -o $@ $^

netstore-server: serwer.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f netstore-client netstore-server
