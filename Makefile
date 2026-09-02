# Compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -Iinclude -If_include -fPIC
# Se corrige LDFLAGS: -rdynamic exporta símbolos globales; -Wl,-rpath,. busca librerías en el directorio actual
LDFLAGS = -rdynamic -ldl -lm -Wl,-rpath,.

# Directorios
SRC_DIR = src
BUILD_DIR = build
SYS_OUT_DIR = f_include/sys/out

# Encontrar recursivamente todos los archivos .c dentro de src/
SRCS = $(shell find $(SRC_DIR) -name "*.c")
OBJS = $(patsubst %, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))

# Binario final
TARGET = foxy

# Módulo nativo sys/out.so
SYS_OUT_SRCS = $(wildcard $(SYS_OUT_DIR)/*.c)
SYS_OUT_SO = sys/out.so

# Regla principal
all: $(BUILD_DIR) sys/out $(TARGET) $(SYS_OUT_SO)

# Crear directorios si no existen
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

sys/out:
	mkdir -p sys/out

# Enlazar el ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Regla de compilación para objetos de foxy
define compile_rule
$(BUILD_DIR)/$(notdir $(1)).o: $(1)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach src, $(SRCS), $(eval $(call compile_rule,$(src))))

# Regla para compilar la librería nativa sys/out.so
$(SYS_OUT_SO): $(SYS_OUT_SRCS)
	$(CC) $(CFLAGS) -shared $(SYS_OUT_SRCS) -o $@

# Limpieza
clean:
	rm -rf $(BUILD_DIR) sys/out.so $(TARGET)

# Phony targets
.PHONY: all clean