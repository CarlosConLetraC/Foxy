#include "f_init.h"
#include "f_dict.h"
#include "f_object.h"

// Declaración de las funciones nativas modularizadas
FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc);

// Punto de entrada del submódulo 'sys.out'
FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    // 1. Crear el diccionario contenedor para 'out'
    FoxyDict *out_dict = f_dict_new();
    if (!out_dict) return;

    // 2. Empaquetar cada función nativa como un FoxyValue
    FoxyValue val_println = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_println };
    FoxyValue val_print   = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_print };
    FoxyValue val_printf  = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_printf };

    // 3. Registrar las funciones dentro del diccionario 'out'
    f_dict_set(out_dict, "println", val_println);
    f_dict_set(out_dict, "print",   val_print);
    f_dict_set(out_dict, "printf",  val_printf);

    // 4. Envolver el diccionario en un FoxyValue y utilizarlo
    FoxyValue out_dict_val = { .type = FOXY_VAL_DICT, .as.dict = out_dict };

    // Registrar o asociar 'out_dict_val' bajo el espacio de nombres global 'sys.out'
    // (Ajusta la función de registro global según la API de tu VM, por ejemplo:)
    // f_vm_set_global(vm, "sys.out", out_dict_val);
    
    // Si manejas los globales mediante una tabla o para evitar el warning temporalmente si se registra en f_init.c raíz:
    (void)out_dict_val; 
}