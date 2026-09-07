#ifndef FOXY_DICT_H
    #define FOXY_DICT_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "uthash.h"

    typedef struct FoxyDictEntry {
        char *key;
        FoxyValue value;
        UT_hash_handle hh;
    } FoxyDictEntry;

    typedef struct FoxyDict {
        FoxyDictEntry *head;
    } FoxyDict;

    FoxyDict* f_dict_new(void);
    void f_dict_free(FoxyDict *dict);

    void f_dict_set(FoxyDict *dict, const char *key, FoxyValue value);
    bool f_dict_get(FoxyDict *dict, const char *key, FoxyValue *out_value);
    bool f_dict_remove(FoxyDict *dict, const char *key);
#endif // FOXY_DICT_H