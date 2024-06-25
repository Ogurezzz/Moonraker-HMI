CC=gcc

CFLAGS=-Og -ggdb -Wall -fdiagnostics-color=always

OBJPATH=obj

BINPATH=bin

all: moonraker-hmi

moonraker-hmi: main.o printer.o jsmn.o
	mkdir -p $(BINPATH)
	$(CC) $(CFLAGS) $(OBJPATH)/main.o $(OBJPATH)/printer.o $(OBJPATH)/jsmn.o -o $(BINPATH)/moonraker-hmi -lcurl

main.o: main.c
	clear
	mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -c main.c -o $(OBJPATH)/main.o

#json.o: json.c
#	$(CC) $(CFLAGS) json.c -o $(OBJPATH)json.o
printer.o: printer.c
	$(CC) $(CFLAGS) -c printer.c -o $(OBJPATH)/printer.o

jsmn.o: jsmn.c
	$(CC) $(CFLAGS) -c jsmn.c -o $(OBJPATH)/jsmn.o

clean:
	rm -rf *.o $(OBJPATH) $(BINPATH)