# ============================================================
# Graph Traversal Demo V2.0 - Makefile
# for MinGW-w64 (GCC) on Windows
# ============================================================

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
LDFLAGS =
TARGET  = graph.exe
SRC     = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo [OK] Build complete: $(TARGET)

clean:
	-del /f $(TARGET) *.o *.bin 2>nul
	@echo Clean complete.

test: $(TARGET)
	@echo === Running built-in tests ===
	./$(TARGET) --test
	@echo === Tests complete ===

run: $(TARGET)
	./$(TARGET)
