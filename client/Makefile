# Makefile для сборки консольного C-клиента

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
LDFLAGS = -lpthread

TARGET  = client

SRC     = src/main.c src/client.c
OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o