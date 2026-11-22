CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = cpm.exe
SRC     = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

clean:
	-del /f /q $(TARGET) 2>nul
	-del /f /q *.bin 2>nul
