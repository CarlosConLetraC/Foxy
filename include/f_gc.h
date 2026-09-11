#ifndef FOXY_GC_H
    #define FOXY_GC_H

    #include "f_settings.h"
    #include <stdbool.h>
    #include <stddef.h>
    // #include <dlfcn.h>
    #include "f_value.h"

    #if FOXY_COMPILER_SUPPORTS_XMACROS
        #define FOXY_GC_COLOR_LIST(F) \
            F(FOXY_GC_COLOR_WHITE) \
            F(FOXY_GC_COLOR_GRAY)  \
            F(FOXY_GC_COLOR_BLACK)
        
        typedef enum {
            #define F(type_enum) type_enum,
            FOXY_GC_COLOR_LIST(F)
            #undef F
        } FoxyGCColor;
    #else // fallback.
        typedef enum {
            FOXY_GC_COLOR_WHITE, // Objeto no visitado (candidato a recolección)
            FOXY_GC_COLOR_GRAY,  // Visitado pero sus referencias no han sido procesadas
            FOXY_GC_COLOR_BLACK  // Visitado y todas sus referencias procesadas
        } FoxyGCColor;
    #endif

    typedef struct FoxyValue FoxyValue;

    typedef struct FoxyGCHandle {
        FoxyValueType type;          // Tipo de valor gestionado por el GC (cadena, objeto, etc.)
        FoxyGCColor color;           // Estado de marcado para el GC
        bool is_pinned;              // Flag para evitar recolección (GC Root temporal)
        
        void *data;                  // Puntero al bloque de datos en heap
        size_t size;                 // Tamaño asignado en bytes (para métricas del GC)

        struct FoxyGCHandle *next;   // Lista enlazada de todos los objetos en el GC
    } FoxyGCHandle;

    // Inicialización y destrucción de handles
    FoxyGCHandle* f_gc_alloc_handle(FoxyState *F, FoxyValueType type, size_t size);
    void f_gc_free_handle(FoxyState *F, FoxyGCHandle *handle);

    // Gestión de marcaje (Tricolor)
    void f_gc_mark_object(FoxyState *F, FoxyGCHandle *handle);
    void f_gc_mark_value(FoxyState *F, FoxyValue value);

    // Ciclo principal de recolección (Mark & Sweep)
    void f_gc_collect(FoxyState *F);
#endif // FOXY_GC_H