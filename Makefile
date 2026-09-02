# Compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -Iinclude -If_include
LDFLAGS = -ldl -lm

# Directorios
SRC_DIR = src
BUILD_DIR = build

# Encontrar recursivamente todos los archivos .c dentro de src/
SRCS = $(shell find $(SRC_DIR) -name "*.c")

# Mapear los fuentes a archivos objeto planos en build/ 
# (asumiendo nombres de archivos únicos en todo src/)
OBJS = $(patsubst %, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))

# Binario final
TARGET = foxy

# Regla principal
all: $(BUILD_DIR) $(TARGET)

# Crear directorio build si no existe
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Enlazar el ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Regla de compilación genérica: busca el .c en cualquier subdirectorio de src/ y compila en build/
define compile_rule
$(BUILD_DIR)/$(notdir $(1)).o: $(1)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach src, $(SRCS), $(eval $(call compile_rule,$(src))))

# Limpieza
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean