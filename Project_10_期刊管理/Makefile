# ============================================================
# Journal Management System V2.0 - Makefile
# for MinGW-w64 (GCC) / Windows
# ============================================================

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = journal.exe
SRC = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --test

clean:
	-del /f $(TARGET) *.o *.bin *.exe 2>nul
	-del /f magz_data.bin _test_magz_v2.bin _test_run.bin 2>nul
