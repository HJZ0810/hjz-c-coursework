# ============================================================
# Kindergarten Student Info Management System V2.0 - Makefile
# for MinGW-w64 (GCC) on Windows
# ============================================================

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS =
TARGET = kindergarten.exe
SRC = main.c

.PHONY: all clean run test gcc

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo [OK] Build complete: $(TARGET)

gcc: $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo === Running built-in tests ===
	./$(TARGET) -t
	@echo === Tests complete ===

clean:
	-del /f $(TARGET) *.o *.dat students.dat 2>nul
