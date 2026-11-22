# 高考志愿填报信息管理系统 V2.0 - Makefile
# 编译器: D:/bin/gcc.exe
# 标准: C99
# 编译选项: -Wall -Wextra -O2 零警告

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
LDFLAGS =
TARGET  = gaokao.exe
SOURCE  = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)
	@echo.
	@echo [SUCCESS] 编译完成: $(TARGET)
	@echo 编译选项: $(CFLAGS)

test: $(TARGET)
	@echo.
	@echo ========================================
	@echo 运行内置自测 (20项)
	@echo ========================================
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

help:
	@echo 高考志愿填报信息管理系统 V2.0
	@echo.
	@echo 可用目标:
	@echo   make all      - 编译项目 (默认)
	@echo   make test     - 编译并运行自测
	@echo   make run      - 编译并运行程序
	@echo   make clean    - 清理编译产物
	@echo   make help     - 显示帮助

clean:
	@-del /f $(TARGET) 2>nul
	@-del /f gaokao_v2.dat 2>nul
	@echo [CLEAN] 已清理
