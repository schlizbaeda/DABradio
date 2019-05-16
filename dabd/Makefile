CC=g++
CFLAGS=-ggdb -Wall
LDFLAGS=-L/usr/lib
LIBRARIES=-lkeystonecomm -lpthread
SRC=dabd.cpp
OBJECTS=dabd.o
EXEC=dabd

$(EXEC) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXEC) $(LDFLAGS) $(LIBRARIES)

$(OBJECTS) : $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	rm -rf *.o $(EXEC)
