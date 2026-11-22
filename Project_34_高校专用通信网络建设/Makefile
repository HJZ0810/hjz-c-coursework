CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = net.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /Q $(TARGET) 2&gt;nul
	-del /Q *.netv2 2&gt;nul
	-del /Q test_*.bin 2&gt;nul

test: $(TARGET)
	.\$(TARGET) --test

run: $(TARGET)
	.\$(TARGET)
