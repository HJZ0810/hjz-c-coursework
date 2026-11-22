# 软件公司客户关系管理系统 V2.0 - Makefile
# 编译器：D:/bin/gcc.exe (MinGW GCC)
# 标准：C99，警告全开，O2优化

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = crm.exe
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
	-del /f $(TARGET) 2>nul
	-del /f crmv2.dat 2>nul
	-del /f bad_test.dat 2>nul
