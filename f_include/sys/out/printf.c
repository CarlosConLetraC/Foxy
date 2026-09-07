#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_object.h"
#include "f_class.h"
#include "f_dict.h"
#include "f_function.h"
#include "f_methods.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *obj, int args) {
    (void)obj;

    if (!vm || args < 1) return;

    // Obtener el proceso activo de la VM
    FoxyProcess *p = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!p) return;

    // El argumento 0 (cadena de formato) está en el fondo del marco de este llamado
    FoxyValue fmt_val = f_vm_peek(p, (size_t)(args - 1));
    char *allocated_fmt = NULL;
    const char *format = NULL;

    if (fmt_val.type == FOXY_VAL_ARRAY) {
        allocated_fmt = (char*)f_value_get_char_array_data(&fmt_val);
        format = allocated_fmt;
    } else if (fmt_val.type == FOXY_VAL_OBJECT && fmt_val.as.obj) {
        format = (const char*)fmt_val.as.obj;
    }

    if (!format) {
        if (allocated_fmt) free(allocated_fmt);
        return;
    }

    int arg_index = 1; 
    const char *ptr = format;
    char buffer[FOXY_NAME_BUFFER_SIZE];
    int len = 0;

    while (*ptr != '\0') {
        if (*ptr == '%') {
            ptr++;

            if (*ptr == '%') {
                f_utils_syswrite(1, "%", 1);
                ptr++;
                continue;
            }

            if (arg_index >= args) {
                f_utils_syswrite(1, "%", 1);
                if (*ptr != '\0') {
                    f_utils_syswrite(1, ptr, 1);
                    ptr++;
                }
                continue;
            }

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

            size_t distance = (size_t)(args - 1 - arg_index);
            FoxyValue val = f_vm_peek(p, distance);

            switch (*ptr) {
                case 'T': {
                    // %T: Imprime cualquier tipo de dato Foxy
                    f_utils_print_constant_dynamic(*(FoxyConstant*)&val, precision);
                    break;
                }
                case 'd':
                case 'i': {
                    int64_t num = 0;
                    if (f_value_is_numeric(&val)) {
                        num = (int64_t)f_value_as_double(&val);
                    } else if (val.type == FOXY_VAL_BOOL) {
                        num = val.as.boolean ? 1 : 0;
                    }
                    len = snprintf(buffer, sizeof(buffer), "%" PRId64, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'u':
                case 'x':
                case 'X': {
                    uint64_t num = 0;
                    if (f_value_is_numeric(&val)) {
                        num = (uint64_t)f_value_as_double(&val);
                    } else if (val.type == FOXY_VAL_BOOL) {
                        num = val.as.boolean ? 1ULL : 0ULL;
                    }

                    const char *fmt_str = (*ptr == 'u') ? "%" PRIu64 : ((*ptr == 'x') ? "%" PRIx64 : "%" PRIX64);
                    len = snprintf(buffer, sizeof(buffer), fmt_str, num);
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'b': {
                    bool b = false;
                    if (val.type == FOXY_VAL_BOOL) {
                        b = val.as.boolean;
                    } else if (f_value_is_numeric(&val)) {
                        b = (f_value_as_double(&val) != 0.0);
                    } else {
                        b = (val.as.ptr != NULL);
                    }
                    f_utils_syswrite(1, b ? "true" : "false", b ? 4 : 5);
                    break;
                }
                case 'f': {
                    double num = f_value_as_double(&val);
                    if (precision >= 0) {
                        len = snprintf(buffer, sizeof(buffer), "%.*f", precision, num);
                    } else {
                        len = snprintf(buffer, sizeof(buffer), "%f", num);
                    }
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
                    break;
                }
                case 'c': {
                    char c = (char)f_value_as_double(&val);
                    f_utils_syswrite(1, &c, 1);
                    break;
                }
                case 's': {
                    char *allocated_str = NULL;
                    const char *str = NULL;

                    if (val.type == FOXY_VAL_ARRAY) {
                        allocated_str = (char*)f_value_get_char_array_data(&val);
                        str = allocated_str;
                    } else if (val.type == FOXY_VAL_OBJECT && val.as.obj) {
                        str = (const char*)val.as.obj;
                    } else {
                        str = f_utils_get_string_from_constant(*(FoxyConstant*)&val);
                    }

                    if (str) {
                        f_utils_syswrite(1, str, strlen(str));
                    } else {
                        f_utils_syswrite(1, "(null)", 6);
                    }

                    if (allocated_str) free(allocated_str);
                    break;
                }
                case 'O': {
                    if (val.type == FOXY_VAL_OBJECT && val.as.obj) {
                        FoxyClass *klass = (FoxyClass*)val.as.obj->klass;
                        const char *cname = (klass && klass->name) ? klass->name : "Object";
                        len = snprintf(buffer, sizeof(buffer), "<Object:%s>", cname);
                    } else if (val.type == FOXY_VAL_CLASS && val.as.klass) {
                        FoxyClass *klass = (FoxyClass*)val.as.klass;
                        len = snprintf(buffer, sizeof(buffer), "<Class:%s>", (klass->name) ? klass->name : "Anon");
                    } else if (val.type == FOXY_VAL_DICT) {
                        len = snprintf(buffer, sizeof(buffer), "<Dict:%p>", (void*)val.as.dict);
                    } else {
                        len = snprintf(buffer, sizeof(buffer), "<NullObject>");
                    }
                    if (len > 0) f_utils_syswrite(1, buffer, (size_t)len);
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
            if (*ptr != '\0') ptr++;
        } else {
            f_utils_syswrite(1, ptr, 1);
            ptr++;
        }
    }

    if (allocated_fmt) free(allocated_fmt);

    fflush(stdout);
}