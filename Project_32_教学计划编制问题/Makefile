CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = sched.exe

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

test: $(TARGET)
	./$(TARGET) --test

clean:
	-del $(TARGET) 2>nul
	-del *.dat 2>nul

.PHONY: all test clean
