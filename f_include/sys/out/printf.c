#include "f_init.h"
#include <stdio.h>

// Funciones auxiliares para extraer tipos desde FoxyValue
static const char* get_string_from_value(FoxyValue val) {
    if (val.type == FOXY_VAL_OBJECT && val.as.obj) {
        return (const char *)val.as.obj;
    }
    return NULL;
}

static int64_t get_long_from_value(FoxyValue val) {
    if (val.type == FOXY_VAL_INT) {
        return val.as.ival;
    } else if (val.type == FOXY_VAL_BOOL) {
        return val.as.bval ? 1 : 0;
    }
    return 0;
}

static double get_double_from_value(FoxyValue val) {
    if (val.type == FOXY_VAL_INT) {
        return (double)val.as.ival;
    }
    return 0.0;
}

FOXY_EXPORT FoxyValue f_sys_out_printf(int argc, FoxyValue *args) {
    if (argc < 1 || !args) return FOXY_NULL_VALUE;

    // En la nueva arquitectura, args[0] es la cadena de formato
    const char *format = get_string_from_value(args[0]);
    if (!format) {
        return FOXY_NULL_VALUE;
    }

    int current_arg_idx = 1; // Los argumentos a formatear empiezan desde args[1] en adelante

    while (*format != '\0') {
        if (*format == '%') {
            format++; // Consumir '%'
            
            if (*format == '\0') {
                putchar('%');
                break;
            }

            char specifier = *format;

            if (specifier == '%') {
                // Literal '%%'
                putchar('%');
                format++;
                continue;
            }

            if (current_arg_idx < argc) {
                FoxyValue arg_val = args[current_arg_idx];

                switch (specifier) {
                    case 'd': {
                        int64_t val = get_long_from_value(arg_val);
                        printf("%ld", (long)val);
                        current_arg_idx++;
                        break;
                    }
                    case 's': {
                        const char *str_val = get_string_from_value(arg_val);
                        if (str_val) {
                            printf("%s", str_val);
                        } else {
                            printf("[object]");
                        }
                        current_arg_idx++;
                        break;
                    }
                    case 'f': {
                        double f_val = get_double_from_value(arg_val);
                        printf("%f", f_val);
                        current_arg_idx++;
                        break;
                    }
                    case 'T': {
                        // Impresión dinámica según el tipo de FoxyValue
                        if (arg_val.type == FOXY_VAL_INT) {
                            printf("%ld", (long)arg_val.as.ival);
                        } else if (arg_val.type == FOXY_VAL_OBJECT && arg_val.as.obj) {
                            printf("%s", (const char *)arg_val.as.obj);
                        } else if (arg_val.type == FOXY_VAL_BOOL) {
                            printf("%s", arg_val.as.bval ? "true" : "false");
                        } else {
                            printf("nil");
                        }
                        current_arg_idx++;
                        break;
                    }
                    default: {
                        putchar('%');
                        putchar(specifier);
                        break;
                    }
                }
            } else {
                // Si faltan argumentos en el stack para el especificador
                putchar('%');
                putchar(specifier);
            }
            format++; // Consumir especificador
        } 
        else if (*format == '\\') {
            format++; // Consumir '\'
            
            if (*format == '\0') {
                putchar('\\');
                break;
            }

            char target_char;
            switch (*format) {
                case 'n': target_char = '\n'; break;
                case 'r': target_char = '\r'; break;
                case 't': target_char = '\t'; break;
                case 'a': target_char = '\a'; break;
                case 'b': target_char = '\b'; break;
                case 'v': target_char = '\v'; break;
                case 'f': target_char = '\f'; break;
                case '\\': target_char = '\\'; break;
                case '\"': target_char = '\"'; break;
                case '\'': target_char = '\''; break;
                default:  target_char = *format; break;
            }
            
            putchar(target_char);
            format++;
        } 
        else {
            putchar(*format);
            format++;
        }
    }

    fflush(stdout);
    return FOXY_NULL_VALUE;
}