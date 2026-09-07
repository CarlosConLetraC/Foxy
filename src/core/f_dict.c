#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_dict.h"
#include "f_gc.h"

FoxyDict* f_dict_new(void) {
    FoxyDict *dict = (FoxyDict*)f_gc_allocate(FOXY_HEAP_DICT, sizeof(FoxyDict), (void(*)(void*))f_dict_free);
    if (!dict) return NULL;
    dict->head = NULL;
    return dict;
}

void f_dict_free(FoxyDict *dict) {
    if (!dict) return;

    FoxyDictEntry *current, *tmp;
    HASH_ITER(hh, dict->head, current, tmp) {
        HASH_DEL(dict->head, current);
        if (current->key) free(current->key);
        f_value_free_contents(&current->value);
        free(current);
    }
}

void f_dict_set(FoxyDict *dict, const char *key, FoxyValue value) {
    if (!dict || !key) return;

    FoxyDictEntry *entry = NULL;
    HASH_FIND_STR(dict->head, key, entry);

    if (entry == NULL) {
        entry = (FoxyDictEntry*)malloc(sizeof(FoxyDictEntry));
        if (!entry) return;

        entry->key = strdup(key);
        entry->value = value;
        HASH_ADD_KEYPTR(hh, dict->head, entry->key, strlen(entry->key), entry);
    } else {
        f_value_free_contents(&entry->value);
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

    HASH_DEL(dict->head, entry);
    if (entry->key) free(entry->key);
    f_value_free_contents(&entry->value);
    free(entry);
    return true;
}