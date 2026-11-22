CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = venue.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

help: $(TARGET)
	./$(TARGET) --help

clean:
	-del /Q $(TARGET) 2>nul || rm -f $(TARGET)
	-del /Q venue_v2.dat 2>nul || rm -f venue_v2.dat
