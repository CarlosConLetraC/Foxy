#ifndef FOXY_GC_H
    #define FOXY_GC_H

    #include <stdbool.h>
    #include <stddef.h>
    #include "f_value.h"

    #define FOXY_HEAP_TYPES(F)                      \
        F(FOXY_HEAP_DICT,               "dict")     \
        F(FOXY_HEAP_OBJECT,             "object")   \
        F(FOXY_HEAP_FUNCTION,           "function") \
        F(FOXY_HEAP_CLASS,              "class")    \
        F(FOXY_HEAP_ENV,                "env")

    #define GENERATE_ENUM(enum_val, string_val) enum_val,
    typedef enum {
        FOXY_HEAP_TYPES(GENERATE_ENUM)
    } FoxyHeapType;
    #undef GENERATE_ENUM

    typedef struct FoxyGCObject {
        FoxyHeapType type;
        bool is_marked;
        struct FoxyGCObject *next;
        void *actual_object;
    } FoxyGCObject;

    void f_gc_init(void);
    void f_gc_shutdown(void);

    void* f_gc_allocate(FoxyHeapType type, size_t size, void (*free_func)(void*));

    void f_gc_mark_value(FoxyValue value);
    void f_gc_mark_root(void *object_ptr);
    void f_gc_collect(void);

    size_t f_gc_get_allocated_count(void);
#endif // FOXY_GC_H