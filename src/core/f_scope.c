#include <stdlib.h>
#include "f_scope.h"

FoxyScope* f_scope_new(FoxyScope *parent) {
    FoxyScope *scope = (FoxyScope*)malloc(sizeof(FoxyScope));
    if (!scope) return NULL;

    scope->parent = parent;
    scope->symbols = NULL; // uthash requiere que la cabecera sea explícitamente NULL
    return scope;
}

void f_scope_free(FoxyScope *scope) {
    if (!scope) return;

    FoxySymbol *current_sym, *tmp;
    // Iteración y liberación segura de la tabla hash de uthash
    HASH_ITER(hh, scope->symbols, current_sym, tmp) {
        HASH_DEL(scope->symbols, current_sym);
        free(current_sym);
    }

    free(scope);
}

bool f_scope_define_by_ptr(FoxyScope *scope, const void *ptr_id, FoxyValue value) {
    if (!scope || !ptr_id) return false;

    FoxySymbol *sym = NULL;
    HASH_FIND_PTR(scope->symbols, &ptr_id, sym);
    
    // Si ya existe en el ámbito actual, actualizamos su valor
    if (sym != NULL) {
        sym->value = value;
        return true;
    }

    sym = (FoxySymbol*)malloc(sizeof(FoxySymbol));
    if (!sym) return false;

    sym->ptr_id = ptr_id;
    sym->value = value;

    HASH_ADD_PTR(scope->symbols, ptr_id, sym);
    return true;
}

FoxyValue* f_scope_lookup_by_ptr(FoxyScope *scope, const void *ptr_id) {
    if (!ptr_id) return NULL;

    FoxyScope *current = scope;
    while (current != NULL) {
        FoxySymbol *sym = NULL;
        HASH_FIND_PTR(current->symbols, &ptr_id, sym);
        if (sym != NULL)
            return &sym->value;
        current = current->parent; // Subir al scope superior si no está localmente
    }

    return NULL; // Variable no encontrada
}

bool f_scope_assign_by_ptr(FoxyScope *scope, const void *ptr_id, FoxyValue value) {
    if (!ptr_id) return false;

    FoxyScope *current = scope;
    while (current != NULL) {
        FoxySymbol *sym = NULL;
        HASH_FIND_PTR(current->symbols, &ptr_id, sym);
        if (sym != NULL) {
            sym->value = value;
            return true;
        }
        current = current->parent;
    }

    return false; // Error: Intento de asignación a variable no declarada
}

bool f_scope_exists_by_ptr(FoxyScope *scope, const void *ptr_id) {
    return f_scope_lookup_by_ptr(scope, ptr_id) != NULL;
}