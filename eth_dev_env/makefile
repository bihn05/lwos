# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = 

# 目标程序名称
TARGET = raw_send

# 源文件
SRCS = main.c

# 对象文件
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 链接目标程序
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 直接运行（需要root权限）
run: $(TARGET)
	sudo ./$(TARGET)

# 运行并捕获输出
test: $(TARGET)
	sudo ./$(TARGET) | tee output.log

# 调试版本（包含更多调试信息）
debug: CFLAGS += -DDEBUG -g -O0
debug: $(TARGET)
	@echo "Debug build done. Run with: sudo ./$(TARGET)"

# 显示帮助信息
help:
	@echo "Available targets:"
	@echo "  all    - Build $(TARGET)"
	@echo "  clean  - Remove built files"
	@echo "  run    - Build and run (requires sudo)"
	@echo "  test   - Build, run and save output"
	@echo "  debug  - Build with debug symbols"
	@echo "  help   - Show this help"

# 伪目标
.PHONY: all clean run test debug help