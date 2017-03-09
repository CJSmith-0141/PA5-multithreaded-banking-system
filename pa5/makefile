COMPILER = gcc
CFLAGS = -Wall -Werror -g
DEPENDANCIES = bank.h uthash.h
LIBS = -lpthread

all: client server

%.o: %.c $(DEPS)
	$(COMPILER) -c -o $@ $< $(CFLAGS)


client: client.o
	$(COMPILER) $(CFLAGS) -o client client.o $(LIBS)

server: server.o bank.o
	$(COMPILER) $(CFLAGS) -o server server.o bank.o $(LIBS)
	
client.o: client.c $(DEPENDANCIES)
	$(COMPILER) $(CFLAGS) -c client.c
	
server.o: server.c $(DEPENDANCIES)
	$(COMPILER) $(CFLAGS) -c server.c

bank.o: bank.c $(DEPENDANCIES)
	$(COMPILER) $(CFLAGS) -c bank.c
	

clean:
	rm -f client server *.o
