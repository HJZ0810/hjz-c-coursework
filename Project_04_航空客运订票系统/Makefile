# ============================================================
# Airline Ticket Booking System V2.0 - Makefile
# for MinGW-w64 (GCC) on Windows
# ============================================================

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
LDFLAGS =
TARGET  = airline.exe
SRC     = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo [OK] Build complete: $(TARGET)

clean:
	-del /f $(TARGET) *.o *.dat 2>nul

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo === Help output test ===
	./$(TARGET) --help
	@echo === Test complete ===
