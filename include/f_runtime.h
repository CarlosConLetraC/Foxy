#ifndef F_RUNTIME_H
#define F_RUNTIME_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "f_status.h"
#include "f_value.h"

/* Forward declaration de la VM */
typedef struct FoxyVM FoxyVM;

typedef struct {
    const char *filename;
    size_t line;
    size_t column;
} FoxyLocation;

// #define f_runtime_format_prefix(stream, kind, status) fprintf((stream), "[foxy-%s] (%s): ", (kind), f_status_to_string((status)))

/***
 * Vuelca el rastreo de llamadas (stacktrace) de la VM.
 ***/
void f_runtime_print_backtrace(FoxyVM *vm);

/***
 * Reporta un error de ejecución en la VM.
 ***/
FoxyStatus f_runtime_error(FoxyVM *vm, FoxyStatus status, const char *format, ...);

/***
 * Reporta un error de tipo en operaciones de la VM mediante macro inline.
 ***/
#define f_runtime_type_error(vm, expected, actual, foxop_name) \
    f_runtime_error( \
        (vm), \
        FOXY_STATUS_ERROR_TYPE_MISMATCH, \
        "Operación '%s' esperaba un tipo '%s' pero recibió '%s'", \
        (foxop_name), \
        f_value_type_to_string(expected), \
        f_value_type_to_string(actual) \
    )

/***
 * Reporta un error de compilación/análisis sintáctico.
 ***/
FoxyStatus f_compile_error(FoxyLocation loc, FoxyStatus status, const char *format, ...);

#endif // F_RUNTIME_H