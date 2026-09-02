CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

# Ruta base del sistema (configurable con: make PREFIX=/otra/ruta)
PREFIX ?= /opt/foxy-lang

# Inyectar el macro de la ruta por defecto en el preprocesador C
CFLAGS += -DFOXY_DEFAULT_HOME=\"$(PREFIX)\"

# Archivos del núcleo compartidos
CORE_OBJS = src/core/f_lexer.o src/core/f_utils.o src/core/f_parser.o src/core/f_codegen.o

all: libfoxy.a foxy modules

# 1. Crear la librería estática del Core
libfoxy.a: $(CORE_OBJS)
	ar rcs libfoxy.a $(CORE_OBJS)

# 2. Compilar el intérprete 'foxy' con la VM integrada y soporte dinámico (-ldl)
foxy: src/interpreter/main_foxy.o src/interpreter/f_vm.o libfoxy.a
	$(CC) $(CFLAGS) src/interpreter/main_foxy.o src/interpreter/f_vm.o libfoxy.a -ldl -o foxy

# 3. Compilar los módulos nativos compartidos (.so) en f_include/
modules: f_include/sys.so f_include/sys/out.so

f_include/sys/out.so: f_include/sys/out/init.c f_include/sys/out/print.c f_include/sys/out/printf.c f_include/sys/out/println.c
	$(CC) $(CFLAGS) -shared -fPIC f_include/sys/out/*.c -o f_include/sys/out.so

f_include/sys.so: f_include/sys/init.c
	$(CC) $(CFLAGS) -shared -fPIC f_include/sys/init.c -o f_include/sys.so

# 4. Regla de instalación en /opt/foxy-lang (requiere sudo make install)
install: all
	install -d $(PREFIX)/lib/sys
	install -m 755 f_include/sys.so $(PREFIX)/lib/
	install -m 755 f_include/sys/out.so $(PREFIX)/lib/sys/
	@echo "[Foxy Installer] Módulos desplegados exitosamente en $(PREFIX)/lib"

# Dependencias específicas de los módulos del core
src/interpreter/f_vm.o: src/interpreter/f_vm.c include/f_vm.h include/f_foxcode.h include/f_utils.h
	$(CC) $(CFLAGS) -c src/interpreter/f_vm.c -o src/interpreter/f_vm.o

src/core/f_utils.o: src/core/f_utils.c include/f_utils.h include/f_vm.h
	$(CC) $(CFLAGS) -c src/core/f_utils.c -o src/core/f_utils.o

src/core/f_parser.o: src/core/f_parser.c include/f_parser.h include/f_lexer.h
	$(CC) $(CFLAGS) -c src/core/f_parser.c -o src/core/f_parser.o

src/core/f_codegen.o: src/core/f_codegen.c include/f_codegen.h include/f_parser.h include/f_foxcode.h
	$(CC) $(CFLAGS) -c src/core/f_codegen.c -o src/core/f_codegen.o

# Regla general para compilar archivos .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/core/*.o src/interpreter/*.o src/compiler/*.o libfoxy.a foxy fox f_include/*.so f_include/sys/*.so