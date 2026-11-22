CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = score.exe
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-del /f $(TARGET) *.o *.dat *.txt *.log 2>nul

test: $(TARGET)
	.\$(TARGET) --test

.PHONY: all clean test
