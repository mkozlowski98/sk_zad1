TARGET: netstore-client netstore-server

CC = gcc
CFLAGS = -Wall -O2
LFLAGS = -Wall

netstore-client: klient.o err.o
	$(CC) $(LFLAGS) -o $@ $^

netstore-server: serwer.o err.o
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean TARGET

clean:
	rm -f netstore-client netstore-server klient.o serwer.o err.o
