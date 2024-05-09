# 定义编译器
CC=g++

# 定义编译选项，例如 -g 用于调试信息，-Wall 用于显示所有警告
CFLAGS=-g -Wall

# 定义目标可执行文件
TARGET=main

# 默认目标
all: $(TARGET)

# 定义如何从每个 .cpp 文件生成 .o 文件
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# 定义如何生成最终的可执行文件
$(TARGET): poller.o FutureCompare.o main.o
	$(CC) $(CFLAGS) poller.o FutureCompare.o main.o -o $(TARGET)

# 清理编译生成的文件
clean:
	rm -f *.o $(TARGET)

