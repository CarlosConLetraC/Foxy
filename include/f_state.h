#ifndef F_STATE_H
#define F_STATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "f_lib.h" // encargado de orquestar librerias foxy.
#include "f_value.h"

typedef struct FoxyState FoxyState;

/* Creación y destrucción de estados de VM */
FoxyState* f_state_new_foxy_state(void);
void       f_state_close(FoxyState *F);

/* Carga y ejecución de scripts */
int f_state_dostring(FoxyState *F, const char *str);
int f_state_dofile(FoxyState *F, const char *filename);

/* Manipulación de la Pila / Estado */
void f_state_pushprimitive(FoxyState *F, FoxyValue v); // también funciona con constantes como FOXY_CONSTANT_VALUE_NULL, FOXY_CONSTANT_VALUE_BOOL_TRUE, FOXY_CONSTANT_VALUE_BOOL_FALSE.
void f_state_pushabstract(FoxyState *F, FoxyValue *v); // f_array, f_dict, f_object, f_struct, f_class, f_function, f_enum.
#endif // F_STATE_H