#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "f_vm.h"
#include "f_foxcode.h"
#include "f_foxmode.h"
#include "f_parser.h"
#include "f_utils.h"
#include "f_codegen.h"
#include "f_openlib.h"
#include "f_dict.h"
#include "f_object.h"
#include "f_value.h"
#include "f_function.h"
#include "f_protocol.h"
#include "f_symtable.h"
#include "f_runtime.h"
#include "f_status.h"
#include "f_lib.h"

extern void foxy_init_module(FoxyVM *vm);

// ==========================================
// OPERACIONES DEL STACK
// ==========================================

FoxyValue f_vm_peek(FoxyProcess *p, size_t distance) {
    if (!p || p->stack_top <= distance) {
        fprintf(stderr, "[Foxy VM Error] Stack Peek overflow/underflow (PID %d)\n", p ? (int)p->pid : -1);
        return FOXY_NULL_VALUE;
    }
    return p->stack[p->stack_top - 1 - distance];
}

void f_vm_push(FoxyProcess *p, FoxyValue val) {
    if (!p) return;
    if (p->stack_top >= p->stack_capacity) {
        size_t new_cap = p->stack_capacity == 0 ? 256 : p->stack_capacity * 2;
        FoxyValue *new_stack = (FoxyValue *)realloc(p->stack, sizeof(FoxyValue) * new_cap);
        if (!new_stack) {
            fprintf(stderr, "[Foxy VM Error] Memory allocation failed for proc stack (PID %u)\n", p->pid);
            return;
        }
        p->stack = new_stack;
        p->stack_capacity = new_cap;
    }
    p->stack[p->stack_top++] = val;
}

// Empuja un valor directo a la pila del proceso activo
void f_process_push(FoxyProcess *p, FoxyValue val) { f_vm_push(p, val); }

// Pop del proceso activo
FoxyValue f_process_pop(FoxyProcess *p) { return f_vm_pop(p); }

// Retorna el valor en la pila relativo al tope
FoxyValue f_vm_stack_peek(FoxyProcess *p, size_t dist) { return f_vm_peek(p, dist); }

FoxyVM* f_vm_create(void) {
    FoxyVM *vm = (FoxyVM *)calloc(1, sizeof(FoxyVM));
    if (!vm) return NULL;

    vm->process_capacity = 8;
    vm->processes = (FoxyProcess **)malloc(sizeof(FoxyProcess *) * vm->process_capacity);
    if (!vm->processes) {
        free(vm);
        return NULL;
    }

    vm->loaded_libs_capacity = 4;
    vm->loaded_libs = (char **)malloc(sizeof(char *) * vm->loaded_libs_capacity);
    if (!vm->loaded_libs) {
        free(vm->processes);
        free(vm);
        return NULL;
    }

    vm->running = false;
    return vm;
}

void f_process_free(FoxyProcess* proc) {
    if (!proc) return;
    if (proc->bytecode) free((void *)proc->bytecode);
    if (proc->stack) free(proc->stack);
    if (proc->locals) free(proc->locals);
    free(proc);
}

void f_vm_load_process(FoxyVM *vm, const uint8_t *code, size_t code_size, const char *filename) {
    if (!vm || !code || code_size == 0) return;

    if (vm->process_count >= vm->process_capacity) {
        size_t new_cap = vm->process_capacity == 0 ? 8 : vm->process_capacity * 2;
        FoxyProcess **temp = (FoxyProcess **)realloc(vm->processes, sizeof(FoxyProcess *) * new_cap);
        if (!temp) {
            fprintf(stderr, "[Foxy VM Error] Out of memory expanding proc table\n");
            return;
        }
        vm->processes = temp;
        vm->process_capacity = new_cap;
    }

    FoxyProcess *proc = (FoxyProcess *)calloc(1, sizeof(FoxyProcess));
    if (!proc) return;

    proc->pid = (uint32_t)(vm->process_count + 1);
    snprintf(proc->name, sizeof(proc->name), "%s", filename ? filename : "main_process");
    proc->state = FOXY_PROCESS_READY;

    proc->bytecode = (uint8_t *)malloc(code_size);
    if (!proc->bytecode) {
        free(proc);
        return;
    }
    memcpy((void *)proc->bytecode, code, code_size);
    proc->bytecode_size = code_size;
    proc->ip = 0;

    proc->stack_capacity = 256;
    proc->stack = (FoxyValue *)malloc(sizeof(FoxyValue) * proc->stack_capacity);

    proc->locals_capacity = 16;
    proc->locals = (FoxyValue *)calloc(proc->locals_capacity, sizeof(FoxyValue));
    proc->locals_count = 0;

    if (!proc->stack || !proc->locals) {
        f_process_free(proc);
        return;
    }

    vm->processes[vm->process_count++] = proc;
}

void f_vm_load_script_to_process(FoxyVM *vm, const uint8_t *blob, size_t code_size, const char *proc_name, const char *str_val, size_t target_idx) {
    if (target_idx < vm->constants_capacity) {
        vm->constants[target_idx].type = FOXY_VAL_OBJECT;
        vm->constants[target_idx].as.obj = (FoxyObject *)str_val;
    }

    f_vm_load_process(vm, blob, code_size, proc_name);
}

FoxyValue f_vm_pop(FoxyProcess *p) {
    if (!p || p->stack_top == 0) {
        struct FoxyVM *vm_context = p ? p->vm : NULL;
        
        f_utils_write_runtime_error(
            vm_context, 
            FOXY_TOKEN_ERROR_RUNTIME, 
            "Stack Underflow detectado en el proceso (PID %d)", 
            p ? (int)p->pid : -1
        );
        return FOXY_NULL_VALUE;
    }
    return p->stack[--p->stack_top];
}

void f_vm_stack_pop_n(FoxyVM* vm, size_t n) {
    if (!vm || vm->process_count == 0) return;
    FoxyProcess* proc = vm->processes[vm->current_process_index];
    if (proc->stack_top >= n) 
        proc->stack_top -= n;
    else
        proc->stack_top = 0;
}

// Búsqueda robusta de funciones nativas integrando la tabla de símbolos relacional
FoxyNativeMethod f_vm_find_native(FoxyVM *vm, const char *name) {
    if (!vm || !name) return NULL;

    // 1. Buscar en la tabla de símbolos nativos estáticos del núcleo
    for (size_t i = 0; i < vm->native_symbols_count; i++) {
        if (strcmp(vm->native_symbols[i].name, name) == 0)
            return vm->native_symbols[i].func;
    }

    // 2. Buscar en la tabla de símbolos relacional unificada (`f_symtable`)
    if (vm->symtable) {
        FoxySymbolRow *row = f_symtable_find_by_name(vm->symtable, name);
        if (row && row->value.type == FOXY_VAL_FUNCTION && row->value.as.native_fn != NULL) {
            return (FoxyNativeMethod)row->value.as.native_fn;
        }
    }

    return NULL;
}

FoxyLib* f_vm_get_current_loading_lib(FoxyVM *vm) {
    if (!vm) return NULL;
    return vm->loading_lib;
}

void f_vm_load_module(FoxyVM *vm, const char *raw_module_path) {
    if (!vm || !vm->runtime || !raw_module_path) return;

    // fprintf(stderr, "[Foxy DEBUG] Iniciando carga del módulo '%s' en el runtime...\n", raw_module_path);

    FoxyLib *existing = NULL;
    HASH_FIND_STR(vm->runtime->loadedlibs, raw_module_path, existing);
    if (existing) {
        // fprintf(stderr, "[Foxy DEBUG] El módulo '%s' ya estaba cargado en caché.\n", raw_module_path);
        return; 
    }

    char path_buf[FOXY_NAME_BUFFER_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s.so", raw_module_path);

    void *handle = dlopen(path_buf, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "[Foxy VM Error] No se pudo cargar el módulo '%s': %s\n", path_buf, dlerror());
        return;
    }

    typedef void (*FoxyInitModuleFn)(FoxyVM *);
    FoxyInitModuleFn init_fn = (FoxyInitModuleFn)dlsym(handle, "foxy_init_module");

    FoxyLib *lib = f_lib_new(raw_module_path, handle);
    if (!lib) {
        fprintf(stderr, "[Foxy VM Error] No se pudo instanciar FoxyLib para '%s'\n", raw_module_path);
        dlclose(handle);
        return;
    }

    vm->loading_lib = lib;
    if (init_fn) {
        init_fn(vm); 
    } else {
        fprintf(stderr, "[Foxy VM Warning] La librería '%s' no exporta 'foxy_init_module'\n", path_buf);
    }
    vm->loading_lib = NULL;

    HASH_ADD_KEYPTR(hh, vm->runtime->loadedlibs, raw_module_path, strlen(raw_module_path), lib);
    // fprintf(stderr, "[Foxy DEBUG] Módulo '%s' registrado exitosamente en el runtime.\n", raw_module_path);
}

FoxyStatus f_vm_run(FoxyVM *vm) {
    if (!vm || vm->process_count == 0) return FOXY_STATUS_SUCCESS;

    vm->running = true;
    FoxyProcess *proc = vm->processes[vm->current_process_index];
    proc->state = FOXY_PROCESS_RUNNING;

    #define BUILD_DISPATCH_TABLE(code, name) [code] = &&lbl_##code,
    static void* dispatch_table[] = {
        FOXY_FOXCODE_LIST(BUILD_DISPATCH_TABLE)
    };
    #undef BUILD_DISPATCH_TABLE

    #define DISPATCH() do { \
        if (!vm->running || proc->ip >= (proc->bytecode_size / sizeof(FoxInstruction))) \
            goto lbl_FOXCODE_HALT; \
        inst = code[proc->ip++]; \
        goto *dispatch_table[GET_FOXCODE(inst)]; \
    } while(0)

    FoxInstruction *code = (FoxInstruction *)proc->bytecode;
    FoxInstruction inst;

    DISPATCH();

    lbl_FOXCODE_NOP: {
        DISPATCH();
    }

    lbl_FOXCODE_HALT: {
        proc->running = 0;
        proc->state = FOXY_PROCESS_DEAD;
        vm->running = false;
        return FOXY_STATUS_SUCCESS;
    }

    lbl_FOXCODE_INCLUDE: {
        int const_idx = GETARG_Bx(inst);

        if (const_idx >= (int)vm->constants_count) {
            fprintf(stderr, "[Foxy VM Error] Índice de constante fuera de rango en FOXCODE_INCLUDE (PID %u)\n", proc->pid);
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue path_val = vm->constants[const_idx];
        const char *module_path = NULL;

        if (path_val.type == FOXY_VAL_ARRAY || path_val.type == FOXY_VAL_OBJECT) {
            module_path = f_value_get_char_array_data(&path_val);
        } else if (path_val.type == FOXY_VAL_CHAR) {
            module_path = path_val.as.sval ? path_val.as.sval : path_val.as.string;
        }

        if (module_path) {
            f_vm_load_module(vm, module_path);
        } else {
            fprintf(stderr, "[Foxy VM Error] FOXCODE_INCLUDE requiere una ruta de módulo válida (PID %u)\n", proc->pid);
            proc->state = FOXY_PROCESS_DEAD;
        }
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_CONST: {
        int const_idx = GETARG_Bx(inst);
        if (const_idx < (int)vm->constants_count) {
            f_vm_push(proc, vm->constants[const_idx]);
        } else {
            fprintf(stderr, "[Foxy VM Error] Constant index out of bounds: %d\n", const_idx);
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        // fprintf(stdout, "[Foxy VM Debug] procesando FOXCODE_LOAD_CONST en index %i (tipo: %d, ptr: %p). . .\n", const_idx, vm->constants[const_idx].type, vm->constants[const_idx].as.ptr);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_NULL: {
        f_vm_push(proc, FOXY_NULL_VALUE);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_TRUE: {
        FoxyValue val = {0}; // Limpia todos los 8 bytes de la unión a 0
        val.type = FOXY_VAL_BOOL;
        val.as.boolean = true;
        f_vm_push(proc, val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_FALSE: {
        FoxyValue val = {0}; // Limpia todos los 8 bytes de la unión a 0
        val.type = FOXY_VAL_BOOL;
        val.as.boolean = false;
        f_vm_push(proc, val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_LOCAL: {
        int local_idx = GETARG_A(inst);
        if ((size_t)local_idx < proc->locals_capacity) {
            f_vm_push(proc, proc->locals[local_idx]);
        } else {
            fprintf(stderr, "[Foxy VM Error] Local variable index out of bounds: %d\n", local_idx);
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        // fprintf(stdout, "[Foxy VM Debug] cargando variable local[%d] (tipo: %d, ptr: %p). . .\n", local_idx, proc->locals[local_idx].type, proc->locals[local_idx].as.ptr);
        DISPATCH();
    }

    lbl_FOXCODE_STORE_LOCAL: {
        size_t local_idx = (size_t)GETARG_A(inst);
        FoxyValue val = f_vm_pop(proc);

        if (local_idx >= proc->locals_capacity) {
            size_t old_cap = proc->locals_capacity;
            size_t new_cap = old_cap == 0 ? 16 : old_cap * 2;
            while (local_idx >= new_cap) new_cap *= 2;
            
            FoxyValue *new_locals = (FoxyValue *)realloc(proc->locals, sizeof(FoxyValue) * new_cap);
            if (!new_locals) {
                fprintf(stderr, "[Foxy VM Error] Out of memory allocating local variables\n");
                vm->running = false;
                goto lbl_FOXCODE_HALT;
            }

            // Inicializar a cero / NULL el nuevo bloque asignado
            for (size_t i = old_cap; i < new_cap; ++i) {
                new_locals[i] = (FoxyValue){ .type = FOXY_VAL_NULL, .as.ptr = NULL };
            }

            proc->locals = new_locals;
            proc->locals_capacity = new_cap;
        }

        proc->locals[local_idx] = val;
        if (local_idx >= proc->locals_count) {
            proc->locals_count = local_idx + 1;
        }

        // fprintf(stdout, "[Foxy VM Debug] procesando FOXCODE_STORE_LOCAL en index %zu (tipo: %d, ptr: %p). . .\n", local_idx, val.type, val.as.ptr);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_GLOBAL: {
        int global_idx = GETARG_Bx(inst);
        
        if (global_idx < 0 || global_idx >= (int)vm->constants_count) {
            fprintf(stderr, "[Foxy VM Error] Índice fuera de rango en LOAD_GLOBAL: %d\n", global_idx);
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue constant_val = vm->constants[global_idx];
        const char *sym_name = NULL;

        if (constant_val.type == FOXY_VAL_ARRAY || constant_val.type == FOXY_VAL_OBJECT) {
            sym_name = f_value_get_char_array_data(&constant_val);
        } else if (constant_val.type == FOXY_VAL_CHAR) {
            sym_name = constant_val.as.sval ? constant_val.as.sval : constant_val.as.string;
        }

        FoxyValue result_val = FOXY_NULL_VALUE;

        if (sym_name) {
            // 1. Intentar buscar en la tabla relacional unificada (f_symtable)
            if (vm->symtable) {
                FoxySymbolRow *row = f_symtable_find_by_name(vm->symtable, sym_name);
                if (row) {
                    result_val = row->value;
                }
            }

            // 2. Si no se encontró en la tabla relacional, verificar el arreglo de globales vm->globals
            if (result_val.type == FOXY_VAL_NULL && (size_t)global_idx < vm->globals_count) {
                if (vm->globals[global_idx].type != FOXY_VAL_NULL) {
                    result_val = vm->globals[global_idx];
                }
            }

            // 3. Fallback a funciones nativas del runtime base
            if (result_val.type == FOXY_VAL_NULL) {
                FoxyNativeMethod native_fn = f_vm_find_native(vm, sym_name);
                if (native_fn) {
                    result_val.type = FOXY_VAL_FUNCTION;
                    result_val.as.native_fn = (void *)native_fn;
                }
            }

            // Debug log opcional para verificar la resolución
            if (result_val.type == FOXY_VAL_NULL) {
                fprintf(stderr, "[Foxy VM Warning] No se pudo resolver el símbolo global: '%s'\n", sym_name);
            }
        }

        f_vm_push(proc, result_val);
        DISPATCH();
    }

    lbl_FOXCODE_STORE_GLOBAL: {
        int global_idx = GETARG_Bx(inst);
        FoxyValue val = f_vm_pop(proc);

        if ((size_t)global_idx >= vm->globals_capacity) {
            size_t new_cap = vm->globals_capacity == 0 ? 16 : vm->globals_capacity * 2;
            while ((size_t)global_idx >= new_cap) new_cap *= 2;
            FoxyValue *new_globals = realloc(vm->globals, sizeof(FoxyValue) * new_cap);
            if (!new_globals) {
                fprintf(stderr, "[Foxy VM Error] Out of memory allocating globals\n");
                vm->running = false;
                goto lbl_FOXCODE_HALT;
            }
            vm->globals = new_globals;
            vm->globals_capacity = new_cap;
        }

        vm->globals[global_idx] = val;
        if ((size_t)global_idx >= vm->globals_count) {
            vm->globals_count = global_idx + 1;
        }
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_LIB: {
        int lib_name_idx = GETARG_Bx(inst);
        const char *lib_name = NULL;

        if (lib_name_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[lib_name_idx];
            if (c.type == FOXY_VAL_ARRAY && c.as.array) {
                if (c.as.array->element_type_id == FOXY_VAL_CHAR && c.as.array->data) {
                    lib_name = (const char *)c.as.array->data;
                }
            }
        }

        if (lib_name) {
            if (!f_vm_load_library(vm, lib_name)) {
                fprintf(stderr, "[Foxy VM Error] Could not load library: %s\n", lib_name);
                vm->running = false;
                goto lbl_FOXCODE_HALT;
            }
        } else {
            fprintf(stderr, "[Foxy VM Error] Constant index %d is not a valid CHAR array for FOXCODE_LOAD_LIB\n", lib_name_idx);
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_GET_MEMBER: {
        int member_idx = GETARG_Bx(inst);
        const char *member_name = NULL;

        if (member_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[member_idx];
            if (c.type == FOXY_VAL_ARRAY && c.as.array && c.as.array->data) {
                member_name = (const char *)c.as.array->data;
            }
        }

        if (!member_name) {
            fprintf(stderr, "[Foxy VM Error] Índice de constante %d inválido para GET_MEMBER\n", member_idx);
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue target = f_vm_pop(proc);
        FoxyValue result = { .type = FOXY_VAL_NULL };

        if (target.type == FOXY_VAL_DICT && target.as.dict) {
            if (!f_dict_get(target.as.dict, member_name, &result)) {
                fprintf(stderr, "[Foxy VM Error] La clave '%s' no existe en el diccionario\n", member_name);
                proc->running = 0;
                proc->state = FOXY_PROCESS_DEAD;
                goto lbl_FOXCODE_HALT;
            }
        } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
            if (!f_object_get_field(target.as.obj, member_name, &result)) {
                fprintf(stderr, "[Foxy VM Error] El atributo '%s' no existe en el objeto\n", member_name);
                proc->running = 0;
                proc->state = FOXY_PROCESS_DEAD;
                goto lbl_FOXCODE_HALT;
            }
        } else {
            fprintf(stderr, "[Foxy VM Error] Intento de acceder al miembro '%s' en un tipo no válido (%s)\n",
                    member_name, f_value_type_to_char_array(target.type));
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        f_vm_push(proc, result);
        DISPATCH();
    }

    lbl_FOXCODE_SET_MEMBER: {
        int member_idx = GETARG_Bx(inst);
        const char *member_name = NULL;

        if (member_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[member_idx];
            if (c.type == FOXY_VAL_ARRAY && c.as.array && c.as.array->data) {
                member_name = (const char *)c.as.array->data;
            }
        }

        if (!member_name) {
            fprintf(stderr, "[Foxy VM Error] Índice de constante %d inválido para SET_MEMBER\n", member_idx);
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue val_to_assign = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);

        if (target.type == FOXY_VAL_DICT && target.as.dict) {
            f_dict_set(target.as.dict, member_name, val_to_assign);
        } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
            f_object_set_field(target.as.obj, member_name, val_to_assign);
        } else {
            fprintf(stderr, "[Foxy VM Error] Intento de escribir el miembro '%s' en un tipo no mutable (%s)\n",
                    member_name, f_value_type_to_char_array(target.type));
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_ADD: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival + b.as.ival };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_SUB: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival - b.as.ival };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_MUL: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival * b.as.ival };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_DIV: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        if (b.as.ival == 0) {
            fprintf(stderr, "[Foxy VM Error] Division by zero\n");
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival / b.as.ival };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_EQ: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival == b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NEQ: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival != b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_LT: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival < b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_GT: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival > b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_LE: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival <= b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_GE: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival >= b.as.ival) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_FOR_ITER: {
        FoxyValue collection = f_vm_peek(proc, 0);

        if (collection.type != FOXY_VAL_ARRAY && collection.type != FOXY_VAL_DICT) {
            fprintf(stderr, "[Foxy VM Error] Intento de iterar sobre un tipo no válido (%s)\n",
                    f_value_type_to_char_array(collection.type));
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue iter_state = { .type = FOXY_VAL_INT, .as.ival = 0 };
        f_vm_push(proc, iter_state);
        DISPATCH();
    }

    lbl_FOXCODE_FOR_NEXT: {
        size_t exit_offset = (size_t)GETARG_Bx(inst);

        FoxyValue iter_state = f_vm_peek(proc, 0);
        FoxyValue collection = f_vm_peek(proc, 1);

        bool has_next = false;
        FoxyValue current_val = FOXY_NULL_VALUE;

        if (collection.type == FOXY_VAL_ARRAY && collection.as.array) {
            size_t idx = (size_t)iter_state.as.ival;
            if (idx < collection.as.array->length) {
                has_next = true;
                current_val = (FoxyValue){ .type = FOXY_VAL_INT, .as.ival = idx };
                proc->stack[proc->stack_top - 1].as.ival = idx + 1;
            }
        }

        if (has_next) {
            f_vm_push(proc, current_val);
        } else {
            proc->ip = exit_offset;
        }
        DISPATCH();
    }

    lbl_FOXCODE_FOREACH_CALL: {
        uint8_t callback_args_count = (uint8_t)GETARG_A(inst);
        FoxyValue callback = f_vm_pop(proc);
        (void)callback_args_count;
        (void)callback;
        DISPATCH();
    }

    lbl_FOXCODE_JUMP: {
        size_t target_address = (size_t)GETARG_Bx(inst);
        proc->ip = target_address;
        DISPATCH();
    }

    lbl_FOXCODE_JUMP_IF_FALSE: {
        size_t target_address = (size_t)GETARG_Bx(inst);
        FoxyValue condition = f_vm_pop(proc);
        if (!condition.as.boolean) {
            proc->ip = target_address;
        }
        DISPATCH();
    }

    lbl_FOXCODE_POP: {
        f_vm_pop(proc); // O el puntero FoxyProcess activo en ese ámbito
        DISPATCH();
    }

    lbl_FOXCODE_CALL: {
        int arg_count = GETARG_A(inst);
        FoxyValue callee_val = f_vm_pop(proc);
        
        if (callee_val.type == FOXY_VAL_FUNCTION && callee_val.as.native_fn != NULL) {
            FoxyNativeMethod native_fn = (FoxyNativeMethod)callee_val.as.native_fn;
            native_fn(vm, NULL, arg_count);
        } else if (callee_val.type == FOXY_VAL_ARRAY && callee_val.as.array != NULL) {
            char *func_name = (char *)callee_val.as.array->data;
            if (func_name) {
                FoxyNativeMethod native_fn = f_vm_find_native(vm, func_name);
                if (native_fn) {
                    native_fn(vm, NULL, arg_count);
                } else {
                    fprintf(stderr, "[Foxy Runtime Error] Función nativa no encontrada: %s\n", func_name);
                    vm->running = false;
                    goto lbl_FOXCODE_HALT;
                }
            }
        } else {
            fprintf(stderr, "[Foxy Runtime Error] El identificador de llamada no es una función ejecutable (tipo: %d).\n", callee_val.type);
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_RET: {
        if (proc->stack_top == 0) {
            f_vm_push(proc, FOXY_NULL_VALUE);
        }
        proc->state = FOXY_PROCESS_READY;
        return FOXY_STATUS_SUCCESS;
    }

    lbl_FOXCODE_POPEN: {
        FoxyValue env_val   = f_vm_pop(proc);
        FoxyValue name_val  = f_vm_pop(proc);
        FoxyValue fn_val    = f_vm_pop(proc);

        FoxyProtocol *protocol = NULL;
        if (env_val.type == FOXY_VAL_OBJECT) {
            protocol = (FoxyProtocol *)env_val.as.ptr;
        }

        const char *pname = NULL;
        if (name_val.type == FOXY_VAL_ARRAY) {
            pname = f_value_get_char_array_data(&name_val);
        } else {
            pname = "main_subproc";
        }

        if (!fn_val.as.function) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                "POPEN requiere una función válida como callback de bytecode.");
            return FOXY_STATUS_RUNTIME;
        }
        const uint8_t *sub_bytecode = fn_val.as.function->bytecode;

        FoxyProcess *sub_proc = f_process_create(vm->runtime, pname, sub_bytecode, protocol);
        if (!sub_proc) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                "No se pudo instanciar el proceso '%s'", pname);
            return FOXY_STATUS_RUNTIME;
        }

        if (!f_process_start(sub_proc)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                "Fallo al iniciar el hilo del proceso '%s'", pname);
            return FOXY_STATUS_RUNTIME;
        }

        FoxyValue proc_obj;
        proc_obj.type = FOXY_VAL_OBJECT; 
        proc_obj.as.ptr = sub_proc;
        f_vm_push(proc, proc_obj);  
        DISPATCH();
    }

    lbl_FOXCODE_ENV: {
        FoxyValue env_val;
        env_val.type = FOXY_VAL_NULL;
        env_val.as.ptr = NULL;
        f_vm_push(proc, env_val);
        DISPATCH();
    }

    lbl_FOXCODE_ENV_CREATE: {
        FoxyValue env_name_val = f_vm_pop(proc);
        const char *env_name = NULL;

        if (env_name_val.type == FOXY_VAL_ARRAY) {
            env_name = f_value_get_char_array_data(&env_name_val);
        } else {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                "ENV_CREATE requiere un arreglo de caracteres como nombre de protocolo.");
            return FOXY_STATUS_RUNTIME;
        }

        FoxyProtocol *protocol = f_protocol_get_or_create(vm->runtime, env_name);
        if (!protocol) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                "Error al instanciar el SharedEnv '%s'.", env_name ? env_name : "");
            return FOXY_STATUS_RUNTIME;
        }

        FoxyValue prot_val;
        prot_val.type = FOXY_VAL_OBJECT;
        prot_val.as.ptr = protocol;
        f_vm_push(proc, prot_val);
        DISPATCH();
    }

    lbl_FOXCODE_ENV_BIND: {
        FoxyValue env_val  = f_vm_pop(proc);
        FoxyValue proc_val = f_vm_pop(proc);

        if (env_val.type != FOXY_VAL_OBJECT || proc_val.type != FOXY_VAL_OBJECT) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                "ENV_BIND requiere un tipo Objeto (Proceso) y un tipo Objeto (Protocolo) validos.");
            return FOXY_STATUS_RUNTIME;
        }

        FoxyProcess *target_proc = (FoxyProcess *)proc_val.as.ptr;
        FoxyProtocol *protocol   = (FoxyProtocol *)env_val.as.ptr;

        if (target_proc && protocol) {
            target_proc->protocol = protocol;
        } else {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                "Punteros nulos al intentar realizar ENV_BIND.");
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, proc_val);
        DISPATCH();
    }
}

FoxyVM* f_vm_new(void) {
    FoxyVM *vm = (FoxyVM*) malloc(sizeof(FoxyVM));
    if (!vm) return NULL;

    vm->constants = NULL;
    vm->constants_count = 0;
    vm->constants_capacity = 0;

    vm->native_symbols = NULL;
    vm->native_symbols_count = 0;
    vm->native_symbols_capacity = 0;

    vm->loaded_libs = NULL;
    vm->loaded_libs_count = 0;
    vm->loaded_libs_capacity = 0;

    vm->processes = NULL;
    vm->process_count = 0;
    vm->process_capacity = 0;

    vm->runtime = f_runtime_new();
    if (!vm->runtime) {
        free(vm);
        return NULL;
    }
    
    // --- SUBSISTEMA DE TABLA RELACIONAL DE SÍMBOLOS ---
    vm->running = false;
    vm->symtable = f_symtable_new(); // <--- Inicializa la tabla relacional raíz de la VM
    vm->loading_lib = NULL;

    if (!vm->symtable) {
        free(vm);
        return NULL;
    }

    return vm;
}

void f_vm_free(FoxyVM *vm) {
    if (!vm) return;

    if (vm->native_symbols) {
        free(vm->native_symbols);
        vm->native_symbols = NULL;
    }

    if (vm->loaded_libs) {
        for (size_t i = 0; i < vm->loaded_libs_count; i++) {
            if (vm->loaded_libs[i]) {
                free(vm->loaded_libs[i]);
            }
        }
        free(vm->loaded_libs);
        vm->loaded_libs = NULL;
    }
    vm->loaded_libs_count = 0;
    vm->loaded_libs_capacity = 0;

    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            if (vm->constants[i].type == FOXY_VAL_OBJECT && vm->constants[i].as.obj) {
                free(vm->constants[i].as.obj);
            }
        }
        free(vm->constants);
        vm->constants = NULL;
    }
    vm->constants_count = 0;

    if (vm->processes) {
        for (size_t i = 0; i < vm->process_count; i++) {
            if (vm->processes[i]) {
                f_process_free(vm->processes[i]);
            }
        }
        free(vm->processes);
        vm->processes = NULL;
    }
    vm->process_count = 0;

    // Liberar la tabla relacional de símbolos
    if (vm->symtable) {
        f_symtable_free(vm->symtable);
        vm->symtable = NULL;
    }

    free(vm);
}

void f_vm_register_native(FoxyVM *vm, const char *name, FoxyNativeMethod func) {
    if (!vm || !name || !func) return;

    if (vm->native_symbols_count >= vm->native_symbols_capacity) {
        size_t new_cap = vm->native_symbols_capacity == 0 ? 8 : vm->native_symbols_capacity * 2;
        FoxyNativeSymbol *new_syms = (FoxyNativeSymbol *)realloc(vm->native_symbols, sizeof(FoxyNativeSymbol) * new_cap);
        if (!new_syms) return;
        vm->native_symbols = new_syms;
        vm->native_symbols_capacity = new_cap;
    }

    FoxyNativeSymbol *sym = &vm->native_symbols[vm->native_symbols_count++];
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->name[sizeof(sym->name) - 1] = '\0';
    sym->func = func;
}