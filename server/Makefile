# Makefile для сборки серверной части мессенджера
#
# Цели:
#   make        — собрать исполняемый файл server
#   make clean  — удалить скомпилированные файлы

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
LDFLAGS = -lpq -lpthread

TARGET  = server

# Все исходные файлы сервера
SRC     = src/main.c src/logger.c src/thread_pool.c src/client_handler.c

# Автоматически заменяем .c на .o
OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o