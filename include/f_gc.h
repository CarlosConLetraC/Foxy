#ifndef FOXY_GC_H
    #define FOXY_GC_H

    #include "f_settings.h"
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
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
    typedef struct FoxyState FoxyState;
    typedef struct FoxyGCHandle {
        /* Bloque 1: Punteros de 64-bit (16 bytes) */
        struct FoxyGCHandle *next;   // Puntero a la lista global del GC (8 bytes)
        void *data;                  // Puntero al bloque real en heap (8 bytes)

        /* Bloque 2: Entero de 64-bit (8 bytes) */
        size_t size;                 // Tamaño asignado en bytes (8 bytes)

        /* Bloque 3: Campo de bits de 32-bit (4 bytes) */
        uint32_t type      : 8;      // Guardado como entero de 8 bits (FoxyValueType)
        uint32_t color     : 2;      // Estado tricolor: WHITE (0), GRAY (1), BLACK (2)
        uint32_t is_pinned : 1;      // Flag para GC Root temporal
        uint32_t reserved  : 21;     // Reservado para uso futuro

        /* Nota: Se agregarán 4 bytes de padding final para redondear a 32 bytes */
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