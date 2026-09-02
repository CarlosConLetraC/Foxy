# Compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -Iinclude -Isrc/core
LDFLAGS = -ldl -lm

# Directorios
SRC_DIR = src/core
BUILD_DIR = build

# Encontrar automáticamente todos los archivos .c en src/core/
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Mapear los fuentes a archivos objeto dentro de build/
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Binario final
TARGET = foxy

# Regla principal
all: $(BUILD_DIR) $(TARGET)

# Crear directorio de compilación si no existe
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Enlazar el ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Regla de compilación genérica para los objetos (.c -> .o)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpieza de archivos compilados
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean