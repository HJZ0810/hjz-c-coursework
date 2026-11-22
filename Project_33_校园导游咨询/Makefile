CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = guide.exe
SRC = main.c

.PHONY: all clean test help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./$(TARGET) --test

clean:
	-del /F /Q $(TARGET) 2>nul || true
	-del /F /Q campus_v2.dat 2>nul || true

help:
	@echo 校园导游咨询系统 V2.0 - Makefile帮助
	@echo ========================================
	@echo make          - 编译项目
	@echo make test     - 编译并运行自测
	@echo make clean    - 清理编译产物
	@echo make help     - 显示本帮助信息
