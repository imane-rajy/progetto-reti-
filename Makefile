.PHONY: all clean

CC = gcc
CFLAGS = -Wall -Wextra -I./src
SRC_DIR = src

CLIENT_SRC = $(SRC_DIR)/client/client.c $(SRC_DIR)/includes.c
SERVER_SRC = $(SRC_DIR)/server/server.c $(SRC_DIR)/server/lavagna.c $(SRC_DIR)/includes.c

CLIENT_OUT = client
SERVER_OUT = server

all: $(CLIENT_OUT) $(SERVER_OUT)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(CLIENT_OUT): $(CLIENT_SRC) | $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(SERVER_OUT): $(SERVER_SRC) | $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(OUT_DIR)
