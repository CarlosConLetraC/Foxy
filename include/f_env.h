#ifndef F_ENV_H
    #define F_ENV_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_dict.h"

    typedef struct FoxyEnv {
        FoxyDict *bindings;      // Tabla hash interna para las variables del entorno
        struct FoxyEnv *parent;  // Puntero al entorno contenedor/padre (Scoping)
        bool is_marked;          // Control auxiliar del GC
    } FoxyEnv;

    // Ciclo de vida del entorno
    FoxyEnv* f_env_new(FoxyEnv *parent);
    void f_env_free(FoxyEnv *env);

    // Operaciones sobre el entorno
    void f_env_set(FoxyEnv *env, const char *name, FoxyValue value);
    bool f_env_get(FoxyEnv *env, const char *name, FoxyValue *out_value);
#endif // F_ENV_H