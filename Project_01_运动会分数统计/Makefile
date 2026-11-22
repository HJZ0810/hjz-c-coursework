# ============================================================
# Sports Meet Scoring System V2.0 - Makefile
# for MinGW-w64 (GCC) on Windows
# ============================================================

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8
LDFLAGS =
TARGET = sports.exe
SRC = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo [OK] Build complete: $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo === Running built-in tests ===
	./$(TARGET) -t
	@echo === Tests complete ===

clean:
	-del /f $(TARGET) *.o *.dat 2>nul
