#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_env.h"
#include "f_gc.h"

FoxyEnv* f_env_new(FoxyEnv *parent) {
    FoxyEnv *env = (FoxyEnv*)f_gc_allocate(FOXY_HEAP_ENV, sizeof(FoxyEnv), (void(*)(void*))f_env_free);
    if (!env) return NULL;

    env->bindings = f_dict_new();
    env->parent = parent;

    return env;
}

void f_env_free(FoxyEnv *env) {
    if (!env) return;
    // Nota: La memoria interna de env (como bindings) es gestionada y rastreada por el Garbage Collector
}

void f_env_set(FoxyEnv *env, const char *name, FoxyValue value) {
    if (!env || !name || !env->bindings) return;
    f_dict_set(env->bindings, name, value);
}

bool f_env_get(FoxyEnv *env, const char *name, FoxyValue *out_value) {
    if (!env || !name) return false;

    FoxyEnv *current = env;
    while (current != NULL) {
        if (current->bindings && f_dict_get(current->bindings, name, out_value)) {
            return true;
        }
        current = current->parent;
    }

    return false;
}