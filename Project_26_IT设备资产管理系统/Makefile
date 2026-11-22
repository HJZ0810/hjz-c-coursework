CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = asset.exe
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
	-del /f $(TARGET) 2>nul
	-del /f assets.dat 2>nul
	-del /f test_assets.dat 2>nul

