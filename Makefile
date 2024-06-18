CC=gcc

CFLAGS=-Og -ggdb -Wall -fdiagnostics-color=always

OBJPATH=obj

BINPATH=bin

all: Moonraker-HMI

Moonraker-HMI: main.o printer.o
	mkdir -p $(BINPATH)
	$(CC) $(CFLAGS) $(OBJPATH)/main.o $(OBJPATH)/printer.o -o $(BINPATH)/Moonraker-HMI -lcurl

main.o: main.c
	mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -c main.c -o $(OBJPATH)/main.o

#json.o: json.c
#	$(CC) $(CFLAGS) json.c -o $(OBJPATH)json.o
printer.o: printer.c
	$(CC) $(CFLAGS) -c printer.c -o $(OBJPATH)/printer.o

clean:
	rm -rf *.o $(OBJPATH) $(BINPATH)