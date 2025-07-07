CC = gcc
CFLAGS = -Wall -g -std=c99 -Wno-unused-function -finput-charset=UTF-8
LDFLAGS = -lm
INCLUDES = -Iinclude

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = bin/obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/traffic_planner

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) 