# 教师工资信息管理系统 V2.0 Makefile
# 编译器：D:/bin/gcc.exe
# 标准：C99，开启所有警告，O2优化

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = salary.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

help: $(TARGET)
	./$(TARGET) --help

clean:
	-del /f $(TARGET) 2>nul
	-del /f salary_v2.dat 2>nul
	-del /f test_salv2.dat 2>nul
