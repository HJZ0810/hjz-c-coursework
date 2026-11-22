CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = emp.exe
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

clean:
	-del -f $(TARGET) empv2.dat

run: $(TARGET)
	./$(TARGET)

.PHONY: all test clean run
