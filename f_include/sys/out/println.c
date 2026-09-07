#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_object.h"
#include "f_class.h"
#include "f_dict.h"
#include "f_function.h"
#include "f_methods.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc) {
    if (!vm) return;

    FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!proc) return;

    // Si existen argumentos delegamos el recorrido a print()
    if (argc >= 1) {
        f_sys_out_print(vm, self, argc);
    }

    // Salto de línea final
    f_utils_syswrite(1, "\n", 1);
}