// f_callstack.c
#include "f_settings.h"
#include <stdlib.h>
#include "f_callstack.h"

void f_callstack_init(FoxyCallStack *cs) {
    if (!cs) return;
    cs->capacity = FOXY_MAX_CALL_STACK_SIZE;
    cs->count = 0;
    cs->frames = (FoxyCallFrame *)malloc(sizeof(FoxyCallFrame) * cs->capacity);
}

void f_callstack_free(FoxyCallStack *cs) {
    if (!cs) return;
    if (cs->frames) {
        free(cs->frames);
        cs->frames = NULL;
    }
    cs->count = 0;
    cs->capacity = 0;
}

bool f_callstack_push(FoxyCallStack *cs, FoxyFunction *func, const FoxInstruction *ip, size_t stack_base) {
    if (!cs) return false;

    if (cs->capacity == 0) {
        cs->capacity = FOXY_MAX_CALL_STACK_SIZE;
        cs->frames = (FoxyCallFrame *)malloc(sizeof(FoxyCallFrame) * cs->capacity);
        if (!cs->frames) return false;
    }

    if (cs->count >= cs->capacity) {
        size_t new_capacity = cs->capacity * 2;
        FoxyCallFrame *new_frames = (FoxyCallFrame *)realloc(cs->frames, sizeof(FoxyCallFrame) * new_capacity);
        if (!new_frames) return false;
        
        cs->frames = new_frames;
        cs->capacity = new_capacity;
    }

    cs->frames[cs->count] = (FoxyCallFrame){
        .func = func,
        .ip = ip,
        .stack_base = stack_base
    };
    
    cs->count++;
    return true;
}

FoxyCallFrame* f_callstack_pop(FoxyCallStack *cs) {
    if (!cs || cs->count == 0) return NULL;
    cs->count--;
    return &cs->frames[cs->count];
}

FoxyCallFrame* f_callstack_peek(FoxyCallStack *cs) {
    if (!cs || cs->count == 0) return NULL;
    return &cs->frames[cs->count - 1];
}