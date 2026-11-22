CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = stu.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /Q $(TARGET) 2&gt;nul
	-del /Q students.dat test_*.dat 2&gt;nul

test: $(TARGET)
	.\$(TARGET) --test

run: $(TARGET)
	.\$(TARGET)

help:
	@echo ========================================
	@echo   学生学籍管理系统 V2.0 - Makefile帮助
	@echo ========================================
	@echo   make all    - 编译项目
	@echo   make test   - 编译并运行自测
	@echo   make run    - 编译并运行交互模式
	@echo   make clean  - 清理生成文件
	@echo   make help   - 显示此帮助
