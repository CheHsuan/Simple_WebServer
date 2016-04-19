cc ?= gcc
CFLAGS = -O0 -Wall -std=c99
LDFLAGS= -lpthread

EXEC = server

all: $(EXEC)

$(EXEC): socketserver.c ./threadpool/threadpool.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(EXEC)
