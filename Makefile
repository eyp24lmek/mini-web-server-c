CC = gcc
CFLAGS = -Wall -O2
TARGET = server

all: $(TARGET)

$(TARGET): server.c
	$(CC) $(CFLAGS) server.c -o $(TARGET)

clean:
	rm -f $(TARGET)

