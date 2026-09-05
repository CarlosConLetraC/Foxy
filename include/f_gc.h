#ifndef FOXY_GC_H
    #define FOXY_GC_H

    #include <stdbool.h>
    #include <stddef.h>
    #include "f_value.h"

    // Definición de tipos de heap administrados por el GC mediante X-Macros
    #define FOXY_HEAP_TYPES(F)                      \
        F(FOXY_HEAP_DICT,               "dict")     \
        F(FOXY_HEAP_OBJECT,             "object")   \
        F(FOXY_HEAP_FUNCTION,           "function") \
        F(FOXY_HEAP_CLASS,              "class")

    // Generación de la enumeración usando la X-Macro
    #define GENERATE_ENUM(enum_val, string_val) enum_val,
    typedef enum {
        FOXY_HEAP_TYPES(GENERATE_ENUM)
    } FoxyHeapType;
    #undef GENERATE_ENUM

    // Cabecera común o estructura base para objetos del heap administrados
    typedef struct FoxyGCObject {
        FoxyHeapType type;
        bool is_marked;
        struct FoxyGCObject *next;
        void *actual_object; // Puntero al objeto real
    } FoxyGCObject;

    // Ciclo de vida del GC
    void f_gc_init(void);
    void f_gc_shutdown(void);

    // Registro de nuevos objetos en el heap
    void* f_gc_allocate(FoxyHeapType type, size_t size, void (*free_func)(void*));

    // Fases del Garbage Collector
    void f_gc_mark_value(FoxyValue value);
    void f_gc_mark_root(void *object_ptr);
    void f_gc_collect(void);

    // Estadísticas opcionales
    size_t f_gc_get_allocated_count(void);
#endif // FOXY_GC_H