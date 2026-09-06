#ifndef F_CALLSTACK_H
    #define F_CALLSTACK_H

    #include "f_settings.h"
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_function.h"

    typedef struct {
        FoxyFunction *func;       // Función ejecutándose en este marco
        size_t ip;                // Puntero de instrucción actual (offset en el bytecode)
        size_t stack_base;        // Base del stack o variables locales para este marco
    } FoxyCallFrame;

    typedef struct {
        FoxyCallFrame *frames;
        size_t count;
        size_t capacity;
    } FoxyCallStack;

    void f_callstack_init(FoxyCallStack *cs);
    void f_callstack_free(FoxyCallStack *cs);    
    bool f_callstack_push(FoxyCallStack *cs, FoxyFunction *func, size_t ip, size_t stack_base);
    FoxyCallFrame* f_callstack_pop(FoxyCallStack *cs);
    FoxyCallFrame* f_callstack_peek(FoxyCallStack *cs);
#endif // F_CALLSTACK_H