# ============================================================
#  Makefile for Hash Table V2.0
#  Compiler: D:/bin/gcc.exe
#  Standard: C99  |  Flags: -Wall -Wextra -O2
#  Target:   hash.exe
# ============================================================

CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = hash.exe
SRC     = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

help:
	@echo 可用目标:
	@echo   all   - 编译 $(TARGET) (默认)
	@echo   test  - 编译并运行 --test 自测
	@echo   run   - 编译并启动交互模式
	@echo   clean - 删除编译产物
	@echo   help  - 显示本帮助

clean:
	-del /q $(TARGET) 2>nul || true
	-del /q hash_v2.dat 2>nul || true
	-del /q temp_bad.dat 2>nul || true
