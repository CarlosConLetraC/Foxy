#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_env.h"
#include "f_gc.h"
#include "f_dict.h"

FoxyEnv* f_env_new(FoxyEnv *parent) {
    // Asignación controlada por el Garbage Collector en el Heap
    FoxyEnv *env = (FoxyEnv*)f_gc_allocate(FOXY_HEAP_ENV, sizeof(FoxyEnv), (void(*)(void*))f_env_free);
    if (!env) return NULL;

    // El diccionario de bindings también se crea mediante el GC
    env->bindings = f_dict_new();
    env->parent = parent;
    env->is_marked = false;

    return env;
}

void f_env_free(FoxyEnv *env) {
    if (!env) return;

    // Nota: Si f_dict_new ya está registrado en el GC, su propia función f_dict_free 
    // se disparará cuando el GC barra el heap. No obstante, liberarlo de manera segura 
    // o delegarlo al GC evita referencias colgantes.
    if (env->bindings) {
        f_dict_free(env->bindings);
        env->bindings = NULL;
    }

    free(env);
}

void f_env_set(FoxyEnv *env, const char *name, FoxyValue value) {
    if (!env || !name) return;
    // Almacena o actualiza la variable en el diccionario del entorno actual
    f_dict_set(env->bindings, name, value);
}

bool f_env_get(FoxyEnv *env, const char *name, FoxyValue *out_value) {
    if (!env || !name) return false;

    FoxyEnv *current = env;
    while (current != NULL) {
        // Busca en el entorno actual
        if (f_dict_get(current->bindings, name, out_value)) {
            return true;
        }
        // Si no está, asciende al entorno padre (lexical scope)
        current = current->parent;
    }

    return false; // Variable no encontrada en la jerarquía de entornos
}