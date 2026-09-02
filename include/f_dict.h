#ifndef FOXY_DICT_H
    #define FOXY_DICT_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "uthash.h" // Asegúrate de tener uthash.h en tu include path

    // Entrada individual del diccionario compatible con uthash
    typedef struct FoxyDictEntry {
        char *key;              // Clave de la cadena
        FoxyValue value;        // Valor asociado
        UT_hash_handle hh;      // Manejador interno de uthash para hacer la estructura hasheable
    } FoxyDictEntry;

    typedef struct FoxyDict {
        FoxyDictEntry *head;    // Puntero a la cabeza de la tabla hash (requerido por uthash, inicializar en NULL)
        bool is_marked;         // Bandera para el Garbage Collector (GC)
        struct FoxyDict *next;  // Puntero para la lista enlazada global del GC
    } FoxyDict;

    // Ciclo de vida del diccionario
    FoxyDict* f_dict_new(void);
    void f_dict_free(FoxyDict *dict);

    // Operaciones clave-valor
    void f_dict_set(FoxyDict *dict, const char *key, FoxyValue value);
    bool f_dict_get(FoxyDict *dict, const char *key, FoxyValue *out_value);
    bool f_dict_remove(FoxyDict *dict, const char *key);
#endif // FOXY_DICT_H