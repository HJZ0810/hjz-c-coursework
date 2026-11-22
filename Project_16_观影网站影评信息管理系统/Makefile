CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lm
TARGET = movie.exe
SRC = main.c

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo 编译成功: $(TARGET)

test: $(TARGET)
	./$(TARGET) --test

run: $(TARGET)
	./$(TARGET)

clean:
	-del /F /Q $(TARGET) 2>nul || true
	-del /F /Q *.dat 2>nul || true
	-del /F /Q _test_*.dat 2>nul || true
	@echo 清理完成

help:
	@echo 可用目标:
	@echo   all    - 编译项目 (默认)
	@echo   test   - 编译并运行测试
	@echo   run    - 编译并运行
	@echo   clean  - 清理生成的文件
	@echo   help   - 显示此帮助
