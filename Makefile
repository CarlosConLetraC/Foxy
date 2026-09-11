# Compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -O2 -fomit-frame-pointer -Iinclude -If_include -fPIC -Wsign-conversion
LDFLAGS = -rdynamic -ldl -lm -Wl,-rpath,.

# Directorios
SRC_DIR = src
BUILD_DIR = build
SYS_OUT_DIR = f_include/sys/out
SYS_OUT_BIN_DIR = sys/out

# Clasificación de fuentes del núcleo y puntos de entrada
CORE_SRCS = $(shell find $(SRC_DIR)/core -name "*.c")
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS))

FOXY_MAIN_SRC = $(SRC_DIR)/interpreter/main_foxy.c
FOXY_MAIN_OBJ = $(BUILD_DIR)/interpreter/main_foxy.o

FOX_MAIN_SRC = $(SRC_DIR)/compiler/main_foxc.c
FOX_MAIN_OBJ = $(BUILD_DIR)/compiler/main_foxc.o

# Binarios finales del motor
TARGET_FOXY = foxy
TARGET_FOX = fox

# Módulos nativos individuales (.so) dentro de sys/out/
SYS_OUT_SRCS = $(wildcard $(SYS_OUT_DIR)/*.c)
SYS_OUT_SOS  = $(patsubst $(SYS_OUT_DIR)/%.c, $(SYS_OUT_BIN_DIR)/%.so, $(SYS_OUT_SRCS))

# Regla principal por defecto
all: $(BUILD_DIR) $(SYS_OUT_BIN_DIR) $(TARGET_FOXY) $(TARGET_FOX) $(SYS_OUT_SOS)

# Objetivos directos
foxy: $(TARGET_FOXY)
fox: $(TARGET_FOX)

# Crear directorios de salida
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SYS_OUT_BIN_DIR):
	mkdir -p $(SYS_OUT_BIN_DIR)

# Regla de compilación genérica para objetos de src/ en build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Enlazar el intérprete (foxy)
$(TARGET_FOXY): $(CORE_OBJS) $(FOXY_MAIN_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Enlazar el compilador (fox)
$(TARGET_FOX): $(CORE_OBJS) $(FOX_MAIN_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Regla para compilar CADA módulo nativo en su propio .so individual
$(SYS_OUT_BIN_DIR)/%.so: $(SYS_OUT_DIR)/%.c | $(SYS_OUT_BIN_DIR)
	$(CC) $(CFLAGS) -shared $< -o $@

# Limpieza completa
clean:
	rm -rf $(BUILD_DIR) $(SYS_OUT_BIN_DIR) $(TARGET_FOXY) $(TARGET_FOX)

# Phony targets
.PHONY: all clean foxy fox