# Nome dos executáveis
CLIENT = client
SERVER = server

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -D_POSIX_C_SOURCE=200112L

# Arquivos fonte e cabeçalhos
COMMON_SRC = common.c
COMMON_HEADER = common.h
CLIENT_SRC = client.c $(COMMON_SRC)
SERVER_SRC = server.c $(COMMON_SRC)

# Regras principais
all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

# Limpeza
clean:
	rm -f $(CLIENT) $(SERVER)
