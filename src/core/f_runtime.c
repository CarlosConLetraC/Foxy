#include "f_runtime.h"

void f_runtime_print_backtrace(FoxyVM *vm) {
    if (!vm || vm->frame_count == 0) {
        fprintf(stderr, "Traceback (sin fotogramas activos en la pila de llamadas).\n");
        return;
    }

    fprintf(stderr, "Traceback (últimas llamadas procesadas, orden descendente):\n");

    /* Iterar desde el fotograma más reciente (top) hasta la raíz */
    for (int i = (int)vm->frame_count - 1; i >= 0; i--) {
        FoxyCallFrame *frame = &vm->frames[i];
        FoxyFunction *func = frame->function;

        const char *func_name = (func && func->name) ? func->name : "<script/anonymous>";
        const char *filename = (frame->loc.filename) ? frame->loc.filename : "<unknown>";

        fprintf(
            stderr,
            "  [%d] En '%s()' -> %s:%zu:%zu\n",
            i,
            func_name,
            filename,
            frame->loc.line,
            frame->loc.column
        );
    }
}

FoxyStatus f_runtime_error(FoxyVM *vm, FoxyStatus status, const char *format, ...) {
    va_list args;
    f_runtime_format_prefix(stderr, "runtime-error", status);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    if (vm) f_runtime_print_backtrace(vm);
    return status;
}

FoxyStatus f_runtime_compile_error(FoxyLocation loc, FoxyStatus status, const char *format, ...) {
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