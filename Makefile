CC = gcc                                           # 编译器
CFLAGS = -Wall -g -std=c99 -Wno-unused-function    # 编译选项 -Wall 显示所有警告 -g 生成调试信息 -std=c99 使用C99标准 -Wno-unused-function 忽略未使用的函数警告
LDFLAGS = -lm                                      # 链接选项 -lm 链接数学库

SRC_DIR = src                                      # 源文件目录
INCLUDE_DIR = include                              # 头文件目录
OBJ_DIR = bin/obj                                  # 对象文件目录
BIN_DIR = bin                                      # 可执行文件目录

TARGET = $(BIN_DIR)/traffic_planner                # 目标文件

SRCS = $(wildcard $(SRC_DIR)/*.c)                  # 源文件列表
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS)) # 对象文件列表

.PHONY: all clean                                  # 伪目标 all 和 clean

all: $(TARGET)                                     # 目标 all 依赖于目标文件

$(TARGET): $(OBJS)                                  # 目标 $(TARGET) 依赖于对象文件 $(OBJS)
	@mkdir -p $(BIN_DIR)                            # 创建可执行文件目录
	$(CC) $(OBJS) -o $@ $(LDFLAGS)                  # 链接对象文件生成可执行文件

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c                       # 规则 将源文件编译为对象文件
	@mkdir -p $(OBJ_DIR)                             # 创建对象文件目录
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@     # 编译源文件生成对象文件

clean:                                             # 目标 clean
	rm -rf $(OBJ_DIR) $(BIN_DIR)                    # 删除对象文件和可执行文件