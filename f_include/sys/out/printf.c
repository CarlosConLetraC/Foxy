#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;

    if (argc < 1 || vm->process_count == 0) {
        return;
    }

    FoxyProcess *p = vm->processes[vm->current_process_index];

    // El argumento 0 de la llamada es el format string
    FoxyValue fmt_val = f_vm_peek(p, (size_t)(argc - 1));
    if (fmt_val.type != FOXY_VAL_ARRAY) {
        return;
    }

    const char *format = f_value_get_char_array_data(&fmt_val);
    if (!format) {
        return;
    }

    int arg_index = 1; 
    const char *ptr = format;
    char buffer[256];
    int len = 0;

    while (*ptr != '\0') {
        if (*ptr == '%') {
            ptr++;

            // Secuencia '%%': Imprimir '%' literal vía syswrite
            if (*ptr == '%') {
                f_utils_syswrite(1, "%", 1);
                ptr++;
                continue;
            }

            // Si se acabaron los argumentos pasados al printf
            if (arg_index >= argc) {
                f_utils_syswrite(1, "%", 1);
                if (*ptr != '\0') {
                    f_utils_syswrite(1, ptr, 1);
                    ptr++;
                }
                continue;
            }

            // Parsear precisión flotante (ej. %.2f)
            int precision = -1;
            if (*ptr == '.') {
                ptr++;
                precision = 0;
                while (*ptr >= '0' && *ptr <= '9') {
                    precision = precision * 10 + (*ptr - '0');
                    ptr++;
                }
                if (precision > 99) precision = 99;
            }

            // Obtener el valor de la pila de la VM
            size_t distance = (size_t)(argc - 1 - arg_index);
            FoxyConstant val = f_vm_peek(p, distance);

            // Manejo por cada especificador de formato
            switch (*ptr) {
                case 'T': {
                    // %T: Comodín universal / ToString dinámico usando f_utils
                    f_utils_print_constant_dynamic(val, precision);
                    break;
                }
                case 'd':
                case 'i': {
                    int64_t num = 0;
                    switch (val.type) {
                        case FOXY_VAL_INT:
                        case FOXY_VAL_LONG:
                            num = val.as.ival;
                            break;
                        case FOXY_VAL_FLOAT:
                            num = (int64_t)val.as.fval; // Conversión implícita estilo Lua
                            break;
                        case FOXY_VAL_DOUBLE:
                        case FOXY_VAL_NUMBER:
                            num = (int64_t)val.as.dval; // Conversión implícita de double
                            break;
                        default:
                            num = val.as.ival;
                            break;
                    }
                    len = snprintf(buffer, sizeof(buffer), "%" PRId64, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'u': {
                    uint64_t num = (uint64_t)val.as.ival;
                    len = snprintf(buffer, sizeof(buffer), "%" PRIu64, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'x': {
                    uint64_t num = (uint64_t)val.as.ival;
                    len = snprintf(buffer, sizeof(buffer), "%" PRIx64, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'X': {
                    uint64_t num = (uint64_t)val.as.ival;
                    len = snprintf(buffer, sizeof(buffer), "%" PRIX64, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'b': {
                    bool b = (val.type == FOXY_VAL_BOOL) ? val.as.boolean : (val.as.ival != 0);
                    if (b) {
                        f_utils_syswrite(1, "true", 4);
                    } else {
                        f_utils_syswrite(1, "false", 5);
                    }
                    break;
                }
                case 'f': {
                    double num = (val.type == FOXY_VAL_INT || val.type == FOXY_VAL_LONG) 
                                 ? (double)val.as.ival 
                                 : val.as.fval;
                    if (precision >= 0) {
                        len = snprintf(buffer, sizeof(buffer), "%.*f", precision, num);
                    } else {
                        len = snprintf(buffer, sizeof(buffer), "%f", num);
                    }
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'c': {
                    char c = (char)val.as.ival;
                    f_utils_syswrite(1, &c, 1);
                    break;
                }
                case 's': {
                    const char *str = f_utils_get_string_from_constant(val);
                    if (str) {
                        f_utils_syswrite(1, str, strlen(str));
                    } else if (val.as.object) {
                        f_utils_syswrite(1, (const char*)val.as.object, strlen((const char*)val.as.object));
                    } else {
                        f_utils_syswrite(1, "(null)", 6);
                    }
                    break;
                }
                default: {
                    f_utils_syswrite(1, "%", 1);
                    if (*ptr != '\0') {
                        f_utils_syswrite(1, ptr, 1);
                    }
                    break;
                }
            }

            arg_index++;
            if (*ptr != '\0') {
                ptr++;
            }
        } else {
            f_utils_syswrite(1, ptr, 1);
            ptr++;
        }
    }

    fflush(stdout);
}