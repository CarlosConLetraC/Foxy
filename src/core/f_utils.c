#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "f_utils.h"
#include "f_value.h"

long f_utils_syswrite(int fd, const char *buf, unsigned long count);

// Extrae una cadena de texto desde un FoxyConstant
const char* f_utils_get_string_from_constant(FoxyConstant c) {
    // Las cadenas se manejan bajo FOXY_VAL_ARRAY (tipo 10) u OBJECT según la especificación del VM
    if (c.type != FOXY_VAL_ARRAY && c.type != FOXY_VAL_OBJECT)
        return NULL;
    
    // Retorna el puntero extrayéndolo del campo de la unión correspondiente a objetos/arreglos
    return (const char *)c.as.obj; // o c.as.array según tu estructura de unión
}

// Extrae un valor numérico entero (long / int64_t) desde un FoxyConstant
long f_utils_get_long_from_constant(FoxyConstant constant) {
    if (constant.type == FOXY_VAL_INT)
        return (long)constant.as.ival;
    return 0L;
}

// Extrae un valor de punto flotante de doble precisión (double) desde un FoxyConstant
double f_utils_get_double_from_constant(FoxyConstant constant) {
    if (constant.type == FOXY_VAL_FLOAT)
        return (double)constant.as.ival;
    return 0.0;
}
// Imprime de forma dinámica cualquier tipo de FoxyConstant usando f_utils_syswrite

void f_utils_print_constant_dynamic(FoxyValue constant) {
    char buffer[64];
    int len = 0;

    switch (constant.type) {
        case FOXY_VAL_NULL:
            f_utils_syswrite(1, "null", 4);
            break;

        case FOXY_VAL_CHAR:
            buffer[0] = constant.as.cval;
            f_utils_syswrite(1, buffer, 1);
            break;

        case FOXY_VAL_INT:
            len = snprintf(buffer, sizeof(buffer), "%" PRId32, constant.as.ival);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_NUMBER:
            len = snprintf(buffer, sizeof(buffer), "%g", constant.as.numval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_FLOAT:
            len = snprintf(buffer, sizeof(buffer), "%f", (double)constant.as.fval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_DOUBLE:
            len = snprintf(buffer, sizeof(buffer), "%lf", constant.as.dval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_LONG:
            len = snprintf(buffer, sizeof(buffer), "%ld", constant.as.lval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_LONG_LONG:
            len = snprintf(buffer, sizeof(buffer), "%lld", constant.as.llval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_UNSIGNED_LONG_LONG:
            len = snprintf(buffer, sizeof(buffer), "%llu", constant.as.ullval);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_BOOL:
            if (constant.as.bval)
                f_utils_syswrite(1, "true", 4);
            else
                f_utils_syswrite(1, "false", 5);
            break;

        case FOXY_VAL_ARRAY:
            // Si el array contiene datos de texto (cadena)
            if (constant.as.array && constant.as.array->data)
                f_utils_syswrite(1, (const char*)constant.as.array->data, constant.as.array->length);
            else
                f_utils_syswrite(1, "[]", 2);
            break;

        case FOXY_VAL_OBJECT:
        case FOXY_VAL_DICT:
        case FOXY_VAL_STRUCT:
            if (constant.as.obj) {
                len = snprintf(buffer, sizeof(buffer), "<object %p>", (void*)constant.as.obj);
                if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "null", 4);
            }
            break;

        case FOXY_VAL_FUNCTION:
            if (constant.as.func) {
                len = snprintf(buffer, sizeof(buffer), "<fn %p>", (void*)constant.as.func);
                if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "<fn null>", 9);
            }
            break;

        case FOXY_VAL_CLASS:
            if (constant.as.klass) {
                len = snprintf(buffer, sizeof(buffer), "<class %p>", (void*)constant.as.klass);
                if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "<class null>", 12);
            }
            break;

        case FOXY_VAL_COUNT:
        default:
            f_utils_syswrite(1, "<unknown>", 9);
            break;
    }
}

static const char* f_get_home(void) {
    const char* env_home = getenv("FOXY_HOME");
    if (env_home && env_home[0] != '\0') return env_home;

    #ifdef FOXY_DEFAULT_HOME
        return FOXY_DEFAULT_HOME;
    #else
        return "/opt/foxy-lang";
    #endif
}

void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name) {
    char real_path[1024];
    const char* home = f_get_home();

    // 1. Intentar ruta principal bajo FOXY_HOME/lib/
    snprintf(real_path, sizeof(real_path), "%s/lib/%s.so", home, lib_name);
    void* handle = dlopen(real_path, RTLD_NOW | RTLD_LOCAL);

    // 2. Fallback a ruta local de desarrollo (f_include/) si no se halla en /opt/
    if (!handle) {
        snprintf(real_path, sizeof(real_path), "f_include/%s.so", lib_name);
        handle = dlopen(real_path, RTLD_NOW | RTLD_LOCAL);
    }

    if (!handle) {
        fprintf(stderr, "[Foxy Loader Error] No se pudo cargar la librería '%s': %s\n", lib_name, dlerror());
        return;
    }

    // 3. Buscar punto de entrada y registrar
    typedef int (*foxy_init_fn)(FoxyVM* vm);
    foxy_init_fn init_module = (foxy_init_fn)dlsym(handle, "foxy_module_init");

    if (init_module) {
        init_module(vm);
        printf("[Foxy VM] Módulo nativo cargado: %s\n", real_path);
    } else {
        fprintf(stderr, "[Foxy Loader Error] El módulo no exporta 'foxy_module_init'\n");
    }
}

char* f_utils_read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", filepath);
        return NULL;
    }

    // Ir al final del archivo para medir su tamaño
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Reservar memoria para el contenido + terminador nulo '\0'
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memoria insuficiente para leer '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    // Leer el archivo completo al búfer
    size_t read_bytes = fread(buffer, 1, length, file);
    if (read_bytes != (size_t)length) {
        fprintf(stderr, "Error: No se pudo leer el archivo completo.\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0'; // Asegurar terminación de cadena en C
    fclose(file);
    return buffer;
}

int f_utils_int_to_ascii(long num, char *buf, unsigned long buffer_size) {
    if (buffer_size == 0) return 0;

    size_t i = 0;
    int is_negative = 0;

    if (num == 0) {
        if (buffer_size < 2) return 0;
        buf[0] = '0';
        return 1;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        if (i >= buffer_size - 1) return -1;
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        if (i >= buffer_size - 1) return -1;
        buf[i++] = '-';
    }

    for (size_t j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = temp;
    }

    return i;
}

// Llamada cruda al kernel para escribir en stdout (fd = 1)
long f_utils_syswrite(int fd, const char *buf, unsigned long count) {
    register long rax __asm__("rax") = 1;
    register long rdi __asm__("rdi") = fd;
    register long rsi __asm__("rsi") = (long)buf;
    register long rdx __asm__("rdx") = count;

    __asm__ __volatile__ (
        "syscall"
        : "+r" (rax)
        : "r" (rdi), "r" (rsi), "r" (rdx)
        : "memory", "cc", "r11", "rcx"
    );
    return rax;
}