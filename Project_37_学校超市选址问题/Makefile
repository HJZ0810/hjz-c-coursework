CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = loc.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /f $(TARGET) 2>nul
	-del /f campus.locv2 2>nul
	-del /f _test_locv2.tmp 2>nul

test: $(TARGET)
	.\$(TARGET) --test

run: $(TARGET)
	.\$(TARGET)

help:
	@echo ========================================
	@echo   学校超市选址系统 V2.0 Makefile
	@echo ========================================
	@echo   make        - 编译项目
	@echo   make test   - 编译并运行自测
	@echo   make run    - 编译并运行程序
	@echo   make clean  - 清理编译产物
	@echo   make help   - 显示帮助
	@echo ========================================
