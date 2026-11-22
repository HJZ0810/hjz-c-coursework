CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = hosp_v2.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

clean:
	-del /f $(TARGET) hosp_v2.dat 2>nul
