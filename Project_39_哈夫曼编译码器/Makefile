# ============================================================
# Huffman Encoder/Decoder - Makefile
# for MinGW-w64 (GCC) / Linux GCC
# ============================================================

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lm
TARGET = huffman.exe
SRC = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "=== Test 1: Text file compression ==="
	echo "Hello Huffman Coding World!" > test.txt
	./$(TARGET) -c test.txt test.huf
	./$(TARGET) -d test.huf test_out.txt
	@echo "=== Test 2: Verify integrity ==="
	@fc /b test.txt test_out.txt 2>nul || echo "Files differ!"
	@echo "=== Test 3: Archive info ==="
	./$(TARGET) -t test.huf
	@echo "=== Tests complete ==="

clean:
	-del /f $(TARGET) *.o *.huf *.out *.exe test.txt test_out.txt test.huf hfmTree TreePrint huffman.log 2>nul
