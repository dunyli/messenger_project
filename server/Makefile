# Makefile для сборки серверной части мессенджера
# Компилятор: GCC
# Зависимости: libpq (PostgreSQL), pthread (многопоточность)

CC      = gcc                                    # компилятор
CFLAGS  = -Wall -Wextra -pthread                 # флаги: все предупреждения, поддержка потоков
LDFLAGS = -lpq -lpthread                         # библиотеки: PostgreSQL, потоки

TARGET  = server                                 # имя выходного исполняемого файла
SRC     = src/main.c                             # исходные файлы

# Правило по умолчанию — сборка сервера
all: $(TARGET)

# Сборка исполняемого файла из исходников
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Очистка скомпилированных файлов
clean:
	rm -f $(TARGET)