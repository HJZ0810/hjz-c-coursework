# ============================================================
# Library Management System V2.0 - Makefile
# for MinGW-w64 (GCC) / Windows
# ============================================================

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = library.exe
SRC = main.c

.PHONY: all clean run test help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo ========================================
	@echo   Running Library System V2.0 Self-Tests
	@echo ========================================
	./$(TARGET) --test

help: $(TARGET)
	./$(TARGET) --help

clean:
	-del /f $(TARGET) *.o *.dat library.log _test_*.dat 2>nul
	@echo Clean complete.
