#include "f_settings.h"
#include <stdlib.h>
#include "f_callstack.h"

// Inicializa la pila de llamadas con una capacidad inicial por defecto (ej. 8 marcos)
void f_callstack_init(FoxyCallStack *cs) {
    if (!cs) return;
    cs->capacity = FOXY_MAX_STACK_SIZE;
    cs->count = 0;
    cs->frames = malloc(sizeof(FoxyCallFrame) * cs->capacity);
}

// Libera la memoria dinámica reservada para los marcos de la pila
void f_callstack_free(FoxyCallStack *cs) {
    if (!cs) return;
    
    if (cs->frames) {
        free(cs->frames);
        cs->frames = NULL;
    }
    cs->count = 0;
    cs->capacity = 0;
}

// Inserta un nuevo marco de llamada en el tope de la pila (con reasignación dinámica si se llena)
bool f_callstack_push(FoxyCallStack *cs, FoxyFunction *func, size_t ip, size_t stack_base) {
    if (!cs) return false;

    // ¡Blindaje contra capacidades en 0 o punteros no inicializados!
    if (cs->capacity == 0) {
        cs->capacity = FOXY_MAX_CALL_STACK_SIZE; // Capacidad mínima por defecto
        cs->frames = malloc(sizeof(FoxyCallFrame) * cs->capacity);
        if (!cs->frames) return false;
    }

    if (cs->count >= cs->capacity) {
        size_t new_capacity = cs->capacity * 2;
        FoxyCallFrame *new_frames = realloc(cs->frames, sizeof(FoxyCallFrame) * new_capacity);
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

// Extrae y retorna el marco superior de la pila (retorna NULL si está vacía)
FoxyCallFrame* f_callstack_pop(FoxyCallStack *cs) {
    if (!cs || cs->count == 0) return NULL;
    cs->count--;
    return &cs->frames[cs->count];
}

// Consulta el marco superior sin retirarlo de la pila
FoxyCallFrame* f_callstack_peek(FoxyCallStack *cs) {
    if (!cs || cs->count == 0) return NULL;
    return &cs->frames[cs->count - 1];
}