CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = veh.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /f $(TARGET) 2>nul || true
	-del /f vehicles.dat 2>nul || true

test: $(TARGET)
	.\$(TARGET) --test

run: $(TARGET)
	.\$(TARGET)

help:
	@echo 车辆管理系统 V2.0 Makefile
	@echo ========================================
	@echo   make all      - 编译项目
	@echo   make clean    - 清理编译产物
	@echo   make test     - 编译并运行自测
	@echo   make run      - 编译并运行交互模式
	@echo   make help     - 显示此帮助
