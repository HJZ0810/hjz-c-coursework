# ============================================================
# Motor Vehicle Violation Management System V2.0 - Makefile
# for MinGW-w64 (GCC) / Windows
# ============================================================

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = vio.exe
SRC = main.c

.PHONY: all clean run test help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "=== Running built-in self tests ==="
	./$(TARGET) --test
	@echo "=== Tests complete ==="

clean:
	-del /f $(TARGET) *.o *.vio 2>nul
	-del /f _test_*.vio 2>nul

help:
	@echo "Available targets:"
	@echo "  make all    - Build vio.exe (default)"
	@echo "  make run    - Build and run in interactive mode"
	@echo "  make test   - Build and run self tests"
	@echo "  make clean  - Remove build artifacts"
	@echo "  make help   - Show this help"
