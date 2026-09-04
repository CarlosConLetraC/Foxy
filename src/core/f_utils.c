#include "f_settings.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include "f_utils.h"
#include "f_foxcode.h"

// --- Manejo de Errores ---

size_t f_utils_unescape_string(const char *src, size_t src_len, char *dest, size_t dest_size) {
    size_t src_idx = 0;
    size_t dest_idx = 0;

    while (src_idx < src_len) {
        if (dest_idx >= dest_size - 1) {
            return (size_t)-1; // Error: búfer de destino insuficiente
        }

        if (src[src_idx] == '\\' && src_idx + 1 < src_len) {
            src_idx++; // Saltar la barra invertida
            switch (src[src_idx]) {
                case 'n':  dest[dest_idx++] = '\n'; break;
                case 't':  dest[dest_idx++] = '\t'; break;
                case 'r':  dest[dest_idx++] = '\r'; break;
                case '\\': dest[dest_idx++] = '\\'; break;
                case '\"': dest[dest_idx++] = '\"'; break;
                case '\'': dest[dest_idx++] = '\''; break;
                // Puedes agregar más secuencias (hex, unicode, etc.) aquí
                default:
                    // Si el escape no es reconocido, puedes optar por dejarlo tal cual o reportar error
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = src[src_idx];
                    break;
            }
        } else {
            dest[dest_idx++] = src[src_idx];
        }
        src_idx++;
    }

    dest[dest_idx] = '\0'; // Asegurar terminación nula para uso seguro en C
    return dest_idx;
}

const char* f_utils_error_to_string(FoxyErrorType error) {
    switch (error) {
        #define F(code, name) case code: return name;
        FOXY_TOKEN_ERROR_LIST(F)
        #undef F
        default:
            return "UNKNOWN_ERROR";
    }
}

void f_utils_write_runtime_error(struct FoxyVM *vm, FoxyErrorType err_type, const char *format, ...) {
    const char *err_name = f_utils_error_to_string(err_type);
    
    // Imprimir cabecera de error en color rojo (ANSI escape)
    fprintf(stderr, "\033[1;31m[Foxy Runtime Error -> %s]\033[0m ", err_name);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");

    if (vm) {
        // Opcional: actualizar el estado interno de la VM para romper el loop de ejecución
        // vm->status = FOXY_VM_STATUS_ERROR;
    }
}

// --- Carga de Librerías Nativas ---

void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name) {
    void *handle = dlopen(lib_name, RTLD_LAZY);
    if (!handle) {
        f_utils_write_runtime_error(
            vm, 
            FOXY_TOKEN_ERROR_RUNTIME, 
            "No se pudo cargar la librería nativa '%s': %s", 
            lib_name, 
            dlerror()
        );
        return;
    }
    
    // TODO: registrar el handle en la VM para control de memoria / cierre
    // f_vm_register_native_lib(vm, handle);
}

// --- Operaciones sobre FoxyConstant ---

const char* f_utils_get_string_from_constant(FoxyConstant constant) {
    if (constant.type == FOXY_VAL_ARRAY && constant.as.array && constant.as.array->data)
        return (const char *)constant.as.array->data;
    return NULL;
}

long f_utils_get_long_from_constant(FoxyConstant constant) {
    if (constant.type == FOXY_VAL_INT)
        return (long)constant.as.ival;
    if (constant.type == FOXY_VAL_LONG)
        return constant.as.lval;
    return 0L;
}

double f_utils_get_double_from_constant(FoxyConstant constant) {
    if (constant.type == FOXY_VAL_FLOAT)
        return (double)constant.as.fval;
    if (constant.type == FOXY_VAL_DOUBLE)
        return constant.as.dval;
    return 0.0;
}

void f_utils_print_constant_dynamic(FoxyConstant constant) {
    char buffer[64];
    int len = 0;

    switch (constant.type) {
        case FOXY_VAL_NULL:
            f_utils_syswrite(1, "papu", 4);
            break;

        case FOXY_VAL_CHAR:
            buffer[0] = constant.as.cval;
            f_utils_syswrite(1, buffer, 1);
            break;

        case FOXY_VAL_INT:
            len = snprintf(buffer, sizeof(buffer), "%" PRId64, constant.as.ival);
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

        case FOXY_VAL_BOOL:
            if (constant.as.bval)
                f_utils_syswrite(1, "true", 4);
            else
                f_utils_syswrite(1, "false", 5);
            break;

        case FOXY_VAL_ARRAY:
            if (constant.as.array && constant.as.array->data)
                f_utils_syswrite(1, (const char*)constant.as.array->data, constant.as.array->length);
            else
                f_utils_syswrite(1, "[]", 2);
            break;

        case FOXY_VAL_OBJECT:
        case FOXY_VAL_DICT:
        case FOXY_VAL_STRUCT: {
            if (constant.as.obj) {
                const char *type_name = f_value_type_to_string(constant.type);
                int l = snprintf(buffer, sizeof(buffer), "<%s %p>", type_name, (void*)constant.as.obj);
                if (l > 0) f_utils_syswrite(1, buffer, (size_t)l);
            } else {
                f_utils_syswrite(1, "null", 4);
            }
            break;
        }
        
        case FOXY_VAL_FUNCTION: {
            void *fn_ptr = constant.as.func ? (void*)constant.as.func : (void*)constant.as.native_fn;
            const char *type_name = f_value_type_to_string(constant.type);
            int l = snprintf(buffer, sizeof(buffer), "<%s %p>", type_name, fn_ptr);
            if (l > 0) f_utils_syswrite(1, buffer, (size_t)l);
            break;
        }

        case FOXY_VAL_CLASS: {
            if (constant.as.klass) {
                const char *type_name = f_value_type_to_string(constant.type);
                int l = snprintf(buffer, sizeof(buffer), "<%s %p>", type_name, (void*)constant.as.klass);
                if (l > 0) f_utils_syswrite(1, buffer, (size_t)l);
            } else {
                f_utils_syswrite(1, "<class null>", 12);
            }
            break;
        }

        default:
            f_utils_syswrite(1, "<unknown>", 9);
            break;
    }
}

// --- Utilidades del Sistema y Archivos ---
char* f_utils_read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memoria insuficiente para leer '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, length, file);
    if (read_bytes != (size_t)length) {
        fprintf(stderr, "Error: No se pudo leer el archivo completo.\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0';
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
        buf[1] = '\0';
        return 1;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        if (i >= buffer_size - 1) return -1;
        buf[i++] = (char)((num % 10) + '0');
        num /= 10;
    }

    if (is_negative) {
        if (i >= buffer_size - 1) return -1;
        buf[i++] = '-';
    }

    buf[i] = '\0';

    for (size_t j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = temp;
    }

    return (int)i;
}

long f_utils_syswrite(int fd, const char *buf, unsigned long count) {
    #if defined(__linux__) && defined(__x86_64__)
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
    #else
    // Fallback estándar si se compila fuera de Linux x86_64
    return (long)fwrite(buf, 1, count, fd == 1 ? stdout : stderr);
    #endif
}