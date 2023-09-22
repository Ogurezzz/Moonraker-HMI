CC=gcc

CFLAGS=-c -g -Wall -fdiagnostics-color=always

OBJPATH=obj/

BINPATH=bin/

all: Moonraker-HMI

Moonraker-HMI: main.o printer.o
	$(CC) -g $(OBJPATH)main.o $(OBJPATH)printer.o -o $(BINPATH)Moonraker-HMI -lcurl

main.o: main.c
	$(CC) $(CFLAGS) main.c -o $(OBJPATH)main.o

#json.o: json.c
#	$(CC) $(CFLAGS) json.c -o $(OBJPATH)json.o
printer.o: printer.c
	$(CC) $(CFLAGS) printer.c -o $(OBJPATH)printer.o

clean:
	rm -rf *.o $(OBJPATH)*.o $(BINPATH)Moonraker-HMI Moonraker-HMI