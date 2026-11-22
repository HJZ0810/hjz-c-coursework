CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = shop.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /f /q $(TARGET) goods.dat test_shop.dat test_corrupt.dat shop.log sort_by_sales.txt 2>nul

test: $(TARGET)
	.\$(TARGET) --test

run: $(TARGET)
	.\$(TARGET)
