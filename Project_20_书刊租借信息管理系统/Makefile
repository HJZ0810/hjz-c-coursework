# 书刊租借信息管理系统 V2.0 Makefile
# 编译器：D:/bin/gcc.exe
# 标准：C99，开启所有警告，优化等级O2

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = borrow.exe
SRC     = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

clean:
	-del /f $(TARGET) 2>nul
	-del /f *.dat 2>nul
	-del /f *.tmp 2>nul

help:
	@echo ========================================
	@echo  Book Rental V2.0 - Make Targets
	@echo ========================================
	@echo   make        - 编译 borrow.exe
	@echo   make test   - 编译并运行内置自测
	@echo   make run    - 编译并启动交互菜单
	@echo   make clean  - 清理可执行文件和数据文件
	@echo   make help   - 显示本帮助
	@echo ========================================
