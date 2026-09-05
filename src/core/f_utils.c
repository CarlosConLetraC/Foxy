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
                default:
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = src[src_idx];
                    break;
            }
        } else {
            dest[dest_idx++] = src[src_idx];
        }
        src_idx++;
    }

    dest[dest_idx] = '\0';
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
    
    fprintf(stderr, "\033[1;31m[Foxy Runtime Error -> %s]\033[0m ", err_name);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");

    if (vm) {
        // Opcional: actualizar estado de la VM si es necesario
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

void f_utils_print_constant_type(FoxyConstant constant) {
    switch (constant.type) {
        case FOXY_VAL_NULL:
            f_utils_syswrite(1, "Null", 4);
            break;
        case FOXY_VAL_CHAR:
            f_utils_syswrite(1, "Char", 4);
            break;
        case FOXY_VAL_INT:
            f_utils_syswrite(1, "Int", 3);
            break;
        case FOXY_VAL_FLOAT:
            f_utils_syswrite(1, "Float", 5);
            break;
        case FOXY_VAL_DOUBLE:
        case FOXY_VAL_NUMBER:
            f_utils_syswrite(1, "Double", 6);
            break;
        case FOXY_VAL_BOOL:
            f_utils_syswrite(1, "Bool", 4);
            break;
        case FOXY_VAL_ARRAY:
            if (constant.as.array && constant.as.array->element_type_id == FOXY_VAL_CHAR) {
                f_utils_syswrite(1, "String", 6);
            } else {
                f_utils_syswrite(1, "Array", 5);
            }
            break;
        default:
            f_utils_syswrite(1, "Unknown", 7);
            break;
    }
}

void f_utils_print_constant_dynamic(FoxyConstant constant, int precision) {
    char buffer[256];
    int len = 0;

    switch (constant.type) {
        case FOXY_VAL_NULL:
            f_utils_syswrite(1, "null", 4);
            break;

        case FOXY_VAL_CHAR:
            buffer[0] = (char)constant.as.ival;
            f_utils_syswrite(1, buffer, 1);
            break;

        case FOXY_VAL_INT:
            len = snprintf(buffer, sizeof(buffer), "%" PRId64, constant.as.ival);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_FLOAT: {
            double val = (double)constant.as.fval;
            if (precision >= 0 && precision <= 99) {
                len = snprintf(buffer, sizeof(buffer), "%.*f", precision, val);
            } else {
                len = snprintf(buffer, sizeof(buffer), "%f", val);
            }
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;
        }

        case FOXY_VAL_DOUBLE:
        case FOXY_VAL_NUMBER: {
            double val = constant.as.dval;
            if (precision >= 0 && precision <= 99) {
                len = snprintf(buffer, sizeof(buffer), "%.*f", precision, val);
            } else {
                len = snprintf(buffer, sizeof(buffer), "%.6f", val);
            }
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;
        }

        case FOXY_VAL_BOOL:
            if (constant.as.boolean)
                f_utils_syswrite(1, "true", 4);
            else
                f_utils_syswrite(1, "false", 5);
            break;

        case FOXY_VAL_ARRAY:
            if (constant.as.array && constant.as.array->data) {
                if (constant.as.array->element_type_id == FOXY_VAL_CHAR) {
                    f_utils_syswrite(1, (const char*)constant.as.array->data, constant.as.array->length);
                } else {
                    f_utils_syswrite(1, "[array]", 7);
                }
            } else {
                f_utils_syswrite(1, "[]", 2);
            }
            break;

        default:
            f_utils_syswrite(1, "<unknown>", 9);
            break;
    }
}

void f_utils_printf_format(const char *fmt, FoxyConstant *args, size_t arg_count) {
    if (!fmt) return;

    size_t arg_idx = 0;
    const char *p = fmt;

    while (*p != '\0') {
        if (*p == '%' && *(p + 1) != '\0') {
            p++; // Omitir '%'

            // Caso '%%'
            if (*p == '%') {
                f_utils_syswrite(1, "%", 1);
                p++;
                continue;
            }

            int precision = -1;

            // Extracción de precisión: %.0f hasta %.99f
            if (*p == '.') {
                p++;
                precision = 0;
                while (*p >= '0' && *p <= '9') {
                    precision = precision * 10 + (*p - '0');
                    p++;
                }
                if (precision > 99) precision = 99;
            }

            // Consumir el argumento y renderizarlo con la firma exacta de f_utils.h
            if (arg_idx < arg_count) {
                f_utils_print_constant_dynamic(args[arg_idx++], precision);
            }
            p++;
        } else {
            f_utils_syswrite(1, p, 1);
            p++;
        }
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

    unsigned long u_num;
    if (num < 0) {
        is_negative = 1;
        u_num = (unsigned long)(-(num + 1)) + 1;
    } else {
        u_num = (unsigned long)num;
    }

    while (u_num > 0) {
        if (i >= buffer_size - 1) return -1;
        buf[i++] = (char)((u_num % 10) + '0');
        u_num /= 10;
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
    long rax = 1; // SYS_write
    long ret;

    __asm__ __volatile__ (
        "syscall"
        : "=a" (ret)
        : "a" (rax),
          "D" ((long)fd),
          "S" ((long)buf),
          "d" ((long)count)
        : "memory", "rcx", "r11"
    );

    return ret;
    #else
    return (long)fwrite(buf, 1, count, fd == 1 ? stdout : stderr);
    #endif
}

void f_utils_dump_constant_pool(const FoxyValue *constants, size_t count) {
    printf("=== [DEBUG] CONSTANT POOL (%zu elementos) ===\n", count);
    for (size_t i = 0; i < count; i++) {
        switch (constants[i].type) {
            case FOXY_VAL_CHAR:
                if (constants[i].as.ival >= 32 && constants[i].as.ival <= 126) {
                    printf("  [%02zu] CHAR: '%c' (%ld)\n", i, (char)constants[i].as.ival, constants[i].as.ival);
                } else {
                    printf("  [%02zu] CHAR: '\\x%02X' (%ld)\n", i, (unsigned char)constants[i].as.ival, constants[i].as.ival);
                }
                break;
            case FOXY_VAL_ARRAY: {
                FoxyArray *arr = constants[i].as.array;
                if (arr) {
                    if (arr->element_type_id == FOXY_VAL_CHAR && arr->data) {
                        printf("  [%02zu] CHAR ARRAY (String): \"%s\"\n", i, (const char*)arr->data);
                    } else {
                        printf("  [%02zu] GENERIC ARRAY (Elem Type: %d, Length: %zu)\n", 
                               i, arr->element_type_id, arr->length);
                    }
                } else {
                    printf("  [%02zu] ARRAY: null\n", i);
                }
                break;
            }
            case FOXY_VAL_obj: {
                const char *obj_str = constants[i].as.obj ? (const char *)constants[i].as.obj : "null";
                printf("  [%02zu] obj/STRING: \"%s\"\n", i, obj_str);
                break;
            }
            case FOXY_VAL_INT:
                printf("  [%02zu] INT: %ld\n", i, constants[i].as.ival);
                break;
            case FOXY_VAL_FLOAT:
                printf("  [%02zu] FLOAT: %f\n", i, constants[i].as.fval);
                break;
            case FOXY_VAL_NUMBER:
            case FOXY_VAL_DOUBLE:
                printf("  [%02zu] DOUBLE: %f\n", i, constants[i].as.dval);
                break;
            case FOXY_VAL_BOOL:
                printf("  [%02zu] BOOL: %s\n", i, constants[i].as.boolean ? "true" : "false");
                break;
            case FOXY_VAL_NULL:
                printf("  [%02zu] NULL\n", i);
                break;
            default:
                printf("  [%02zu] UNKNOWN TYPE (%d)\n", i, constants[i].type);
                break;
        }
    }
}

void f_utils_dump_bytecode(const FoxInstruction *bytecode, size_t count) {
    printf("\n=== [DEBUG] BYTECODE GENERADO (%zu instrucciones / %zu bytes) ===\n", 
           count, count * sizeof(FoxInstruction));
    for (size_t i = 0; i < count; i++) {
        printf("%08X ", bytecode[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
    if (count % 8 != 0) printf("\n");
    printf("============================================================\n\n");
}