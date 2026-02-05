.PHONY: all clean

CC = gcc
CFLAGS = -Wall -Wextra -I./src -g
SRC_DIR = src

CLIENT_SRC = $(SRC_DIR)/client/client.c $(SRC_DIR)/command.c $(SRC_DIR)/card.c
SERVER_SRC = $(SRC_DIR)/server/server.c $(SRC_DIR)/server/lavagna.c $(SRC_DIR)/command.c $(SRC_DIR)/card.c

CLIENT_OUT = utente 
SERVER_OUT = lavagna

all: $(CLIENT_OUT) $(SERVER_OUT)

$(CLIENT_OUT): $(CLIENT_SRC)
	@printf "Compilo il client...\n"
	@$(CC) $(CFLAGS) $^ -o $@

$(SERVER_OUT): $(SERVER_SRC)
	@printf "Compilo il server...\n"
	@$(CC) $(CFLAGS) $^ -o $@

format:
	@printf "Formatto il sorgente...\n"
	@find . \( -name "*.c" -o -name "*.h" \) -type f -exec clang-format -i {} +

clean:
	@printf "Ripulisco gli oggetti...\n"
	@rm -f $(CLIENT_OUT) $(SERVER_OUT)
