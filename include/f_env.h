#ifndef F_ENV_H
    #define F_ENV_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_dict.h"

    typedef struct FoxyVM FoxyVM;

    typedef struct FoxyEnv {
        FoxyDict *bindings;
        struct FoxyEnv *parent;
    } FoxyEnv;

    FoxyEnv* f_env_new(FoxyEnv *parent);
    void f_env_free(FoxyEnv *env);

    void f_env_set(FoxyEnv *env, const char *name, FoxyValue value, FoxyVM *vm);
    bool f_env_get(FoxyEnv *env, const char *name, FoxyValue *out_value);
#endif // F_ENV_H