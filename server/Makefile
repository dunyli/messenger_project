# Makefile для сборки серверной части мессенджера
#
#   make        — собрать исполняемый файл server
#   make clean  — удалить скомпилированные файлы

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -I/usr/include/postgresql
LDFLAGS = -lpq -lpthread -lssl -lcrypto

TARGET  = server

# Все исходные файлы сервера
SRC = src/main.c src/logger.c src/thread_pool.c src/client_handler.c \
      src/db.c src/crypto.c src/protocol.c src/online_users.c src/websocket.c

OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o