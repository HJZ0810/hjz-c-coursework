CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = warehouse.exe

.PHONY: all clean test

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET) --test

clean:
	-del /f $(TARGET) 2>nul
	-del /f warehouse.dat 2>nul
	-del /f _test_*.dat 2>nul
