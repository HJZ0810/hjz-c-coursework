CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = clubmgr.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /f $(TARGET) clubv2.dat 2>nul

test: $(TARGET)
	$(TARGET) --test

run: $(TARGET)
	$(TARGET)
