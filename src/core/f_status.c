#include <stdio.h>
#include <stdarg.h>
#include "f_status.h"
#include "f_runtime.h"

static const char * const FOXY_STATUS_MESSAGES[] = {
    #define F(code, str) str,
    FOXY_STATUS_LIST(F)
    #undef F
};

const char* f_status_to_string(FoxyStatus status) {
    if (status >= 0 && status < FOXY_STATUS_COUNT) {
        return FOXY_STATUS_MESSAGES[status];
    }
    return "Código de estado desconocido";
}

static void f_runtime_format_prefix(FILE *stream, const char *kind, FoxyStatus status) {
    fprintf(stream, "[foxy-%s] (%s): ", kind, f_status_to_string(status));
}

FoxyStatus f_runtime_error(FoxyVM *vm, FoxyStatus status, const char *format, ...) {
    va_list args;
    f_runtime_format_prefix(stderr, "runtime-error", status);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    if (vm) {
        f_runtime_print_backtrace(vm);
    }

    return status;
}

FoxyStatus f_compile_error(FoxyLocation loc, FoxyStatus status, const char *format, ...) {
    va_list args;
    fprintf(
        stderr,
        "[foxy-compile-error] %s:%zu:%zu (%s): ",
        loc.filename ? loc.filename : "<script>",
        loc.line, loc.column,
        f_status_to_string(status)
    );

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    return status;
}

void f_runtime_print_backtrace(FoxyVM *vm) {
    (void)vm;
    fprintf(stderr, "Traceback (últimas llamadas procesadas):\n  <stacktrace en construcción>\n");
}