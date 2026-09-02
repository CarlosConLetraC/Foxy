#include "f_dict.h"
#include <stdlib.h>
#include <string.h>

FoxyDict* f_dict_new(void) {
    FoxyDict *dict = (FoxyDict*)malloc(sizeof(FoxyDict));
    if (!dict) return NULL;
    dict->head = NULL;      // En uthash, la tabla inicia apuntando a NULL
    dict->is_marked = false;
    dict->next = NULL;
    return dict;
}

void f_dict_free(FoxyDict *dict) {
    if (!dict) return;

    FoxyDictEntry *current, *tmp;
    
    // Macro segura de uthash para iterar y liberar toda la tabla en bucle
    HASH_ITER(hh, dict->head, current, tmp) {
        HASH_DEL(dict->head, current); // Lo saca de la tabla hash
        free(current->key);            // Libera la llave duplicada
        free(current);                 // Libera la entrada
    }
    
    free(dict);
}

void f_dict_set(FoxyDict *dict, const char *key, FoxyValue value) {
    if (!dict || !key) return;

    FoxyDictEntry *entry = NULL;

    // Buscar si la clave ya existe en la tabla hash
    HASH_FIND_STR(dict->head, key, entry);

    if (entry == NULL) {
        // Si no existe, creamos una nueva entrada
        entry = (FoxyDictEntry*)malloc(sizeof(FoxyDictEntry));
        if (!entry) return;

        entry->key = strdup(key);
        entry->value = value;
        
        // Añadir la entrada a la tabla hash de uthash usando la clave string
        HASH_ADD_KEYPTR(hh, dict->head, entry->key, strlen(entry->key), entry);
    } else {
        // Si ya existe, simplemente actualizamos su valor
        entry->value = value;
    }
}

bool f_dict_get(FoxyDict *dict, const char *key, FoxyValue *out_value) {
    if (!dict || !key) return false;
    FoxyDictEntry *entry = NULL;
    HASH_FIND_STR(dict->head, key, entry);
    if (entry == NULL) return false;
    if (out_value) *out_value = entry->value;
    return true;
}

bool f_dict_remove(FoxyDict *dict, const char *key) {
    if (!dict || !key) return false;
    FoxyDictEntry *entry = NULL;
    HASH_FIND_STR(dict->head, key, entry);
    if (entry == NULL) return false;

    // Remover de la tabla hash y liberar memoria
    HASH_DEL(dict->head, entry);
    free(entry->key);
    free(entry);
    return true;
}