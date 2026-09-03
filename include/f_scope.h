#ifndef F_SCOPE_H
    #define F_SCOPE_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "uthash.h"
    #include "f_value.h"

    typedef struct FoxySymbol {
        const void *ptr_id;     // Clave: Dirección de memoria del identificador (%p)
        FoxyValue value;        // Valor almacenado
        UT_hash_handle hh;      // Handle interno de uthash
    } FoxySymbol;

    typedef struct FoxyScope {
        struct FoxyScope *parent; // Ámbito superior (NULL si es global)
        FoxySymbol *symbols;      // Cabecera de la tabla hash (debe iniciar en NULL)
    } FoxyScope;

    // Gestión de ciclo de vida
    FoxyScope* f_scope_new(FoxyScope *parent);
    void f_scope_free(FoxyScope *scope);

    // Operaciones por dirección de memoria (%p)
    bool f_scope_define_by_ptr(FoxyScope *scope, const void *ptr_id, FoxyValue value);
    FoxyValue* f_scope_lookup_by_ptr(FoxyScope *scope, const void *ptr_id);
    bool f_scope_assign_by_ptr(FoxyScope *scope, const void *ptr_id, FoxyValue value);
    bool f_scope_exists_by_ptr(FoxyScope *scope, const void *ptr_id);
#endif // F_SCOPE_H