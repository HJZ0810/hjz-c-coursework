# ============================================================
# Univariate Sparse Polynomial Calculator V2.0 - Makefile
# for MinGW-w64 (GCC) on Windows
# ============================================================

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lm
TARGET  = poly.exe
SRC     = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo [OK] Build complete: $(TARGET)

clean:
	-del /f $(TARGET) *.o *.poly _test.poly 2>nul
	@echo Clean complete.

test: $(TARGET)
	@echo Running built-in self-tests...
	$(TARGET) --test

run: $(TARGET)
	$(TARGET)

help:
	@echo Makefile targets:
	@echo   all    - Build poly.exe (default)
	@echo   clean  - Remove build artifacts
	@echo   test   - Build and run self-tests
	@echo   run    - Build and run interactively
	@echo   help   - Show this help
