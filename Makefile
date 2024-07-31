CC=gcc

CFLAGS=-Og -ggdb -Wall -fdiagnostics-color=always -DDEBUG

OBJPATH=obj

BINPATH=bin

all: moonraker-hmi

moonraker-hmi: main.o printer.o jsmn.o configini.o
	mkdir -p $(BINPATH)
	$(CC) $(CFLAGS) $(OBJPATH)/main.o $(OBJPATH)/printer.o $(OBJPATH)/jsmn.o $(OBJPATH)/configini.o -o $(BINPATH)/moonraker-hmi -lcurl

main.o: main.c
	clear
	mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -c main.c -o $(OBJPATH)/main.o

#json.o: json.c
#	$(CC) $(CFLAGS) json.c -o $(OBJPATH)json.o
printer.o: printer.c
	$(CC) $(CFLAGS) -c printer.c -o $(OBJPATH)/printer.o

jsmn.o: libs/jsmn.c

	$(CC) $(CFLAGS) -c libs/jsmn.c -o $(OBJPATH)/jsmn.o

configini.o: libs/configini.c
	$(CC) $(CFLAGS) -c libs/configini.c -o $(OBJPATH)/configini.o

clean:
	rm -rf *.o $(OBJPATH) $(BINPATH)