#include "f_settings.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include "f_utils.h"
#include "f_foxcode.h"
#include "f_value.h"
#include "f_array.h"
#include "f_class.h"
#include "f_object.h"
#include "f_dict.h"
#include "f_function.h"
#include "f_struct.h"

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

// --- Operaciones sobre FoxyValue (anteriormente FoxyConstant) ---

const char* f_utils_get_string_from_constant(FoxyValue constant) {
    return f_value_get_char_array_data(&constant);
}

long f_utils_get_long_from_constant(FoxyValue constant) {
    if (constant.type == FOXY_VAL_INT || constant.type == FOXY_VAL_LONG || constant.type == FOXY_VAL_LLONG)
        return (long)constant.like.ival;
    return 0L;
}

double f_utils_get_double_from_constant(FoxyValue constant) {
    return f_value_as_double(&constant);
}

void f_utils_print_constant_type(FoxyValue constant) {
    const char *type_name = f_value_type_to_char_array(constant.type);
    f_utils_syswrite(1, type_name, strlen(type_name));
}

void f_utils_print_constant_dynamic(FoxyValue constant, int precision) {
    char buffer[FOXY_NAME_BUFFER_SIZE];
    int len = 0;

    switch (constant.type) {
        case FOXY_VAL_NULL:
            f_utils_syswrite(1, "null", 4);
            break;

        case FOXY_VAL_CHAR:
            buffer[0] = constant.like.cval;
            f_utils_syswrite(1, buffer, 1);
            break;

        case FOXY_VAL_INT:
        case FOXY_VAL_LONG:
        case FOXY_VAL_LLONG:
            len = snprintf(buffer, sizeof(buffer), "%" PRId64, constant.like.ival);
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;

        case FOXY_VAL_FLOAT: {
            double val = constant.like.fval;
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
            double val = constant.like.dval;
            if (precision >= 0 && precision <= 99) {
                len = snprintf(buffer, sizeof(buffer), "%.*f", precision, val);
            } else {
                len = snprintf(buffer, sizeof(buffer), "%.6f", val);
            }
            if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
            break;
        }

        case FOXY_VAL_BOOL:
            if (constant.like.bval || constant.like.boolean)
                f_utils_syswrite(1, "true", 4);
            else
                f_utils_syswrite(1, "false", 5);
            break;

        case FOXY_VAL_ARRAY:
            if (constant.like.array) {
                const char *str_data = f_value_get_char_array_data(&constant);
                if (str_data) {
                    f_utils_syswrite(1, str_data, constant.like.array->count);
                    free((void*)str_data);
                } else {
                    f_utils_syswrite(1, "[array]", 7);
                }
            } else {
                f_utils_syswrite(1, "[]", 2);
            }
            break;

        case FOXY_VAL_DICT:
            f_utils_syswrite(1, "[dict]", 6);
            break;

        case FOXY_VAL_OBJECT:
            if (constant.like.obj && constant.like.obj->klass && constant.like.obj->klass->name) {
                len = snprintf(buffer, sizeof(buffer), "<instance %s>", constant.like.obj->klass->name);
                f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "<object>", 8);
            }
            break;

        case FOXY_VAL_CLASS:
            if (constant.like.klass) {
                FoxyClass *klass = (FoxyClass*)constant.like.klass;
                len = snprintf(buffer, sizeof(buffer), "<class %s>", klass->name ? klass->name : "anonymous");
                f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "<class>", 7);
            }
            break;

        case FOXY_VAL_FUNCTION:
            if (constant.like.func && constant.like.func->name) {
                len = snprintf(buffer, sizeof(buffer), "<fn %s>", constant.like.func->name);
                f_utils_syswrite(1, buffer, (size_t)len);
            } else {
                f_utils_syswrite(1, "<fn>", 4);
            }
            break;

        default:
            f_utils_syswrite(1, "<unknown>", 9);
            break;
    }
}

void f_utils_printf_format(const char *fmt, FoxyValue *args, size_t arg_count) {
    if (!fmt) return;

    size_t arg_idx = 0;
    const char *p = fmt;

    while (*p != '\0') {
        if (*p == '%' && *(p + 1) != '\0') {
            p++; // Omitir '%'

            if (*p == '%') {
                f_utils_syswrite(1, "%", 1);
                p++;
                continue;
            }

            int precision = -1;

            if (*p == '.') {
                p++;
                precision = 0;
                while (*p >= '0' && *p <= '9') {
                    precision = precision * 10 + (*p - '0');
                    p++;
                }
                if (precision > 99) precision = 99;
            }

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
        const FoxyValue *val = &constants[i];
        printf("  [%02zu] %s: ", i, f_value_type_to_char_array(val->type));

        switch (val->type) {
            case FOXY_VAL_INT:
            case FOXY_VAL_LONG:
            case FOXY_VAL_LONG_LONG:
                printf("%lld\n", (long long)val->as.ival);
                break;
            case FOXY_VAL_FLOAT:
            case FOXY_VAL_DOUBLE:
            case FOXY_VAL_NUMBER:
                printf("%f\n", val->as.dval);
                break;
            case FOXY_VAL_BOOL:
                printf("%s\n", (val->as.bval || val->as.boolean) ? "true" : "false");
                break;
            case FOXY_VAL_CHAR:
                printf("'%c'\n", val->as.cval);
                break;
            case FOXY_VAL_ARRAY: {
                const char *str_data = f_value_get_char_array_data(val);
                if (str_data) {
                    printf("CHAR ARRAY (String): \"%s\"\n", str_data);
                    free((void*)str_data);
                } else {
                    printf("ARRAY (count=%zu, length=%zu)\n", 
                           val->as.array ? val->as.array->count : 0,
                           val->as.array ? val->as.array->length : 0);
                }
                break;
            }
            case FOXY_VAL_DICT:
                printf("<dict: %p>\n", val->as.dict);
                break;
            case FOXY_VAL_OBJECT: {
                FoxyObject *obj = val->as.obj;
                if (obj && obj->klass && obj->klass->name) {
                    printf("<object instance of %s: %p>\n", obj->klass->name, (void*)obj);
                } else {
                    printf("<object: %p>\n", (void*)obj);
                }
                break;
            }
            case FOXY_VAL_CLASS: {
                FoxyClass *klass = (FoxyClass*)val->as.klass;
                if (klass && klass->name) {
                    printf("<class: %s>\n", klass->name);
                } else {
                    printf("<class: %p>\n", val->as.klass);
                }
                break;
            }
            case FOXY_VAL_FUNCTION: {
                FoxyFunction *func = val->as.func;
                if (func && func->name) {
                    printf("<function %s arity=%d: %p>\n", func->name, func->arity, (void*)func);
                } else {
                    printf("<function: %p>\n", (void*)func);
                }
                break;
            }
            default:
                printf("<ptr: %p, type: %i>\n", val->as.ptr, val->type);
                break;
        }
    }
    printf("============================================================\n");
}

void f_utils_dump_bytecode(const FoxmodeInstruction *bytecode, size_t count) {
    printf("\n=== [DEBUG] BYTECODE GENERADO (%zu instrucciones / %zu bytes) ===\n", 
           count, count * sizeof(FoxmodeInstruction));
    for (size_t i = 0; i < count; i++) {
        printf("%08X ", bytecode[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
    if (count % 8 != 0) printf("\n");
    printf("============================================================\n\n");
}

bool f_utils_dump_bytecode_to_file(const FoxmodeInstruction *bytecode, size_t code_count, const FoxyVM *vm, const char *filepath) {
    if (!bytecode || code_count == 0 || !filepath) return false;

    FILE *file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error: No se pudo crear el archivo de dump '%s'\n", filepath);
        return false;
    }

    fprintf(file, "======================================================================\n");
    fprintf(file, " FOXY-LANG BYTECODE DUMP\n");
    fprintf(file, " Instrucciones: %zu | Tamaño total: %zu bytes\n", code_count, code_count * sizeof(FoxmodeInstruction));
    fprintf(file, "======================================================================\n\n");

    // 1. Exportación del Pool de Constantes
    if (vm && vm->constants && vm->constants_count > 0) {
        fprintf(file, "--- CONSTANT POOL (%zu elementos) ---\n", vm->constants_count);
        for (size_t i = 0; i < vm->constants_count; i++) {
            const FoxyValue *val = &vm->constants[i];
            const char *type_name = f_value_type_to_char_array(val->type);

            fprintf(file, "[%04zu] Type: %-10s (0x%02X) | Value: ", i, type_name, val->type);

            switch (val->type) {
                case FOXY_VAL_NULL:
                    fprintf(file, "<ptr (nil)>");
                    break;

                case FOXY_VAL_INT:
                case FOXY_VAL_LONG:
                case FOXY_VAL_LONG_LONG:
                    fprintf(file, "%" PRId64, val->as.ival);
                    break;

                case FOXY_VAL_FLOAT:
                case FOXY_VAL_DOUBLE:
                case FOXY_VAL_NUMBER:
                    fprintf(file, "%.6f", val->as.dval);
                    break;

                case FOXY_VAL_BOOL:
                    fprintf(file, "%s", (val->as.bval || val->as.boolean) ? "true" : "false");
                    break;

                case FOXY_VAL_CHAR:
                    fprintf(file, "'%c'", val->as.cval);
                    break;

                case FOXY_VAL_ARRAY: {
                    const char *str_data = f_value_get_char_array_data(val);
                    if (str_data) {
                        fprintf(file, "\"%s\"", str_data);
                        free((void*)str_data);
                    } else if (val->as.array) {
                        fprintf(file, "[Array: count=%zu]", val->as.array->count);
                    } else {
                        fprintf(file, "[]");
                    }
                    break;
                }

                case FOXY_VAL_FUNCTION: {
                    FoxyFunction *func = val->as.func;
                    fprintf(file, "<fn %s (arity %d)>", 
                            (func && func->name) ? func->name : "anonymous", 
                            func ? func->arity : 0);
                    break;
                }

                default:
                    fprintf(file, "<ptr %p>", val->as.ptr);
                    break;
            }

            // Búsqueda del nombre de variable asignado desde vm->symtable
            if (vm->symtable) {
                const char *var_name = f_symtable_get_name_by_value(vm->symtable, val);
                fprintf(file, " | variable name: %s", var_name ? var_name : "<UNKNOWN>");
            }

            fprintf(file, "\n");
        }
        fprintf(file, "\n");
    }

    // 2. Exportación de Instrucciones del Bytecode
    fprintf(file, "--- INSTRUCTION STREAM ---\n");
    fprintf(file, " INDEX  | OFFSET | HEX CODE   | OPCODE | REG A | ARG BX | ANNOTATION\n");
    fprintf(file, "--------+--------+------------+--------+-------+--------+------------------------\n");

    for (size_t i = 0; i < code_count; i++) {
        FoxmodeInstruction inst = bytecode[i];

        FOXY_FOXCODE opcode = GET_FOXCODE(inst);
        int reg_a           = GETARG_A(inst);
        int arg_bx          = GETARG_Bx(inst);

        fprintf(file, " %06zu | %06zu | 0x%08X | %-6u | %-5d | %-6d",
                i, i * sizeof(FoxmodeInstruction), inst, (unsigned int)opcode, reg_a, arg_bx);

        if (vm && vm->constants && arg_bx >= 0 && (size_t)arg_bx < vm->constants_count) {
            const FoxyValue *c_val = &vm->constants[arg_bx];

            if (c_val->type == FOXY_VAL_ARRAY) {
                const char *str = f_value_get_char_array_data(c_val);
                if (str) {
                    fprintf(file, " ; Const[%d] = \"%s\"", arg_bx, str);
                    free((void*)str);
                }
            } else if (f_value_is_pure_integer(c_val)) {
                fprintf(file, " ; Const[%d] = %" PRId64, arg_bx, c_val->as.ival);
            }
        }

        fprintf(file, "\n");
    }

    fprintf(file, "\n======================================================================\n");
    fclose(file);
    return true;
}