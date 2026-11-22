CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = contact.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /Q $(TARGET) 2>nul || true
	-del /Q *.dat 2>nul || true

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

help: $(TARGET)
	./$(TARGET) --help
