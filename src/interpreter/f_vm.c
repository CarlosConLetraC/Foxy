// src/interpreter/f_vm.c
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
#include "f_status.h"
#include "f_function.h"
#include "f_callstack.h"
#include "f_protocol.h"
#include "f_symtable.h"
#include "f_runtime.h"
#include "f_status.h"
#include "f_methods.h"
#include "f_process.h"
#include "f_lib.h"
#include "f_gc.h"
#include "f_array.h"
#include "f_object.h"
#include "f_class.h"
#include "f_methods.h"

extern void foxy_init_module(FoxyVM *vm);

// ==========================================
// OPERACIONES DEL STACK Y PROCESOS
// ==========================================

void f_vm_register_native(FoxyVM *vm, const char *name, FoxyNativeMethod func) {
    if (!vm || !name || !func) return;

    FoxyNativeSymbolEntry *entry = NULL;
    HASH_FIND_STR(vm->native_symbols_hash, name, entry);
    
    if (!entry) {
        entry = (FoxyNativeSymbolEntry *)malloc(sizeof(FoxyNativeSymbolEntry));
        if (!entry) return;
        
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        HASH_ADD_STR(vm->native_symbols_hash, name, entry);
    }
    
    entry->func = func;
}

FoxyProcess* f_vm_get_process(FoxyVM *vm, FoxyProcess *proc_ptr) {
    if (!vm || !vm->processes_hash || !proc_ptr) return NULL;
    
    FoxyProcess *proc = NULL;
    // Búsqueda por la referencia exacta en memoria del objeto
    HASH_FIND_PTR(vm->processes_hash, &proc_ptr, proc);
    return proc;
}

void f_vm_add_process(FoxyVM *vm, FoxyProcess *proc) {
    if (!vm || !proc) return;
    // Se inserta la referencia física del proceso en la tabla hash
    HASH_ADD_PTR(vm->processes_hash, hh.key, proc);
}

FoxyValue f_vm_peek(FoxyProcess *p, size_t distance) {
    if (!p || p->stack_top <= distance) {
        fprintf(stderr, "[Foxy VM Error] Stack Peek overflow/underflow (Proc %p)\n", (void*)p);
        return FOXY_NULL_VALUE;
    }
    return p->stack[p->stack_top - 1 - distance];
}

void f_vm_push(FoxyProcess *p, FoxyValue val) {
    if (!p) return;
    if (p->stack_top >= p->stack_capacity) {
        size_t new_cap = p->stack_capacity == 0 ? FOXY_MAX_FRAMES : p->stack_capacity * 2;
        FoxyValue *new_stack = (FoxyValue *)realloc(p->stack, sizeof(FoxyValue) * new_cap);
        if (!new_stack) {
            fprintf(stderr, "[Foxy VM Error] Memory allocation failed for proc stack (Proc %p)\n", (void*)p);
            return;
        }
        p->stack = new_stack;
        p->stack_capacity = new_cap;
    }
    p->stack[p->stack_top++] = val;
}

// void f_vm_process_push(FoxyProcess *p, FoxyValue val) { f_vm_push(p, val); }
FoxyValue f_vm_stack_peek(FoxyProcess *p, size_t dist) { return f_vm_peek(p, dist); }

FoxyVM* f_vm_create(void) {
    FoxyVM *vm = (FoxyVM *)calloc(1, sizeof(FoxyVM));
    if (!vm) return NULL;

    vm->processes_hash = NULL;
    vm->native_symbols_hash = NULL;

    vm->loaded_libs_capacity = 4;
    vm->loaded_libs = (char **)malloc(sizeof(char *) * vm->loaded_libs_capacity);
    if (!vm->loaded_libs) {
        free(vm);
        return NULL;
    }

    vm->running = false;
    return vm;
}

void f_vm_load_process(FoxyVM *vm, const uint8_t *code, size_t code_size, const char *filename) {
    if (!vm || !code || code_size == 0) return;

    FoxyProcess *proc = (FoxyProcess *)calloc(1, sizeof(FoxyProcess));
    if (!proc) return;

    proc->vm = vm;
    snprintf(proc->name, sizeof(proc->name), "%s", filename ? filename : "main_process");
    proc->state = FOXY_PROCESS_READY;

    proc->stack_capacity = FOXY_MAX_FRAMES;
    proc->stack = (FoxyValue *)malloc(sizeof(FoxyValue) * proc->stack_capacity);

    proc->locals_capacity = FOXY_MAX_LOCALS_CAPACITY;
    proc->locals = (FoxyValue *)calloc(proc->locals_capacity, sizeof(FoxyValue));
    proc->locals_count = 0;

    if (!proc->stack || !proc->locals) {
        f_process_free(proc);
        return;
    }

    f_callstack_init(&proc->call_stack);

    FoxyFunction *main_func = (FoxyFunction *)calloc(1, sizeof(FoxyFunction));
    if (!main_func) {
        f_process_free(proc);
        return;
    }
    main_func->type = FOXY_FUNCTION_USER;
    main_func->as.user.code = (FoxInstruction *)malloc(code_size);
    if (!main_func->as.user.code) {
        free(main_func);
        f_process_free(proc);
        return;
    }
    memcpy(main_func->as.user.code, code, code_size);
    main_func->as.user.code_size = code_size;

    // Se registra la dirección física de la primera instrucción en memoria[cite: 27]
    proc->main_func = main_func;
    if (!f_callstack_push(&proc->call_stack, main_func, main_func->as.user.code, 0)) {
        f_function_free(main_func); 
        f_process_free(proc);
        return;
    }

    f_vm_add_process(vm, proc);
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
            "Stack Underflow detectado en el proceso (%p)", 
            (void*)p
        );
        return FOXY_NULL_VALUE;
    }
    return p->stack[--p->stack_top];
}

void f_vm_stack_pop_n(FoxyVM* vm, size_t n) {
    if (!vm || !vm->processes_hash) return;
    // Obtener el primer proceso en la tabla hash (el proceso raíz o activo)
    FoxyProcess* proc = vm->processes_hash;
    if (!proc) return;

    if (proc->stack_top >= n) 
        proc->stack_top -= n;
    else
        proc->stack_top = 0;
}
FoxyNativeMethod f_vm_find_native(FoxyVM *vm, const char *name) {
    if (!vm || !name) return NULL;

    FoxyNativeSymbolEntry *entry = NULL;
    HASH_FIND_STR(vm->native_symbols_hash, name, entry);
    if (entry) return entry->func;

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

void f_vm_set_current_loading_lib(FoxyVM *vm, FoxyLib *lib) {
    if (vm) vm->loading_lib = lib;
}

void f_vm_load_module(FoxyVM *vm, const char *raw_module_path) {
    if (!vm || !vm->runtime || !raw_module_path) return;

    FoxyLib *existing = NULL;
    HASH_FIND_STR(vm->runtime->loadedlibs, raw_module_path, existing);
    if (existing) return; 

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
    }
    vm->loading_lib = NULL;

    HASH_ADD_KEYPTR(hh, vm->runtime->loadedlibs, raw_module_path, strlen(raw_module_path), lib);
}

static const char * const FOXCODE_SYMBOLS[FOXCODE_COUNT] = {
    #define F(fcode, name, symbol) [fcode] = symbol,
    FOXY_FOXCODE_LIST(F)
    #undef F
};

static inline const char *f_vm_foxcode_to_symbol(FOXY_FOXCODE fcode) {
    if (fcode >= FOXCODE_COUNT) return "UNKNOWN";
    const char *sym = FOXCODE_SYMBOLS[fcode];
    return sym ? sym : "?";
}

static bool f_vm_eval_binary_op(FOXY_FOXCODE fcode, FoxyValue a, FoxyValue b, FoxyValue *out_res) {
    if (!f_value_is_numeric(&a) || !f_value_is_numeric(&b)) return false;

    switch (fcode) {
        case FOXCODE_ADD:
            out_res->type = FOXY_VAL_INT;
            out_res->as.ival = a.as.ival + b.as.ival;
            return true;
        case FOXCODE_SUB:
            out_res->type = FOXY_VAL_INT;
            out_res->as.ival = a.as.ival - b.as.ival;
            return true;
        case FOXCODE_MUL:
            out_res->type = FOXY_VAL_INT;
            out_res->as.ival = a.as.ival * b.as.ival;
            return true;
        case FOXCODE_DIV:
            if (b.as.ival == 0) return false;
            out_res->type = FOXY_VAL_INT;
            out_res->as.ival = a.as.ival / b.as.ival;
            return true;
        case FOXCODE_LT:
            out_res->type = FOXY_VAL_BOOL;
            out_res->as.boolean = (a.as.ival < b.as.ival);
            return true;
        case FOXCODE_GT:
            out_res->type = FOXY_VAL_BOOL;
            out_res->as.boolean = (a.as.ival > b.as.ival);
            return true;
        case FOXCODE_LE:
            out_res->type = FOXY_VAL_BOOL;
            out_res->as.boolean = (a.as.ival <= b.as.ival);
            return true;
        case FOXCODE_GE:
            out_res->type = FOXY_VAL_BOOL;
            out_res->as.boolean = (a.as.ival >= b.as.ival);
            return true;
        default:
            return false;
    }
}

static bool f_vm_eval_unary_bitwise_op(FOXY_FOXCODE fcode, FoxyValue a, FoxyValue *out_res) {
    if (!f_value_is_pure_integer(&a)) return false;
    if (fcode == FOXCODE_BIT_NOT) {
        out_res->type = FOXY_VAL_INT;
        out_res->as.ival = ~a.as.ival;
        return true;
    }
    return false;
}

static bool f_vm_eval_bitwise_op(FOXY_FOXCODE fcode, FoxyValue a, FoxyValue b, FoxyValue *out_res) {
    if (!f_value_is_pure_integer(&a) || !f_value_is_pure_integer(&b)) return false;
    out_res->type = FOXY_VAL_INT;

    switch (fcode) {
        case FOXCODE_BIT_AND: out_res->as.ival = a.as.ival & b.as.ival; return true;
        case FOXCODE_BIT_OR:  out_res->as.ival = a.as.ival | b.as.ival; return true;
        case FOXCODE_BIT_XOR: out_res->as.ival = a.as.ival ^ b.as.ival; return true;
        case FOXCODE_BIT_SHL: out_res->as.ival = a.as.ival << b.as.ival; return true;
        case FOXCODE_BIT_SHR: out_res->as.ival = a.as.ival >> b.as.ival; return true;
        default: return false;
    }
}

// ==========================================
// NÚCLEO DE EJECUCIÓN UNIFICADO
// ==========================================

FoxyStatus f_vm_execute_process(FoxyVM *vm, FoxyProcess *proc) {
    if (!vm || !proc) return FOXY_STATUS_RUNTIME;

    vm->running = true;
    proc->state = FOXY_PROCESS_RUNNING;

    #define BUILD_DISPATCH_TABLE(code, name, str) [code] = &&lbl_##code,
    static void* dispatch_table[] = {
        FOXY_FOXCODE_LIST(BUILD_DISPATCH_TABLE)
    };
    #undef BUILD_DISPATCH_TABLE

    #define DISPATCH() do { \
        FoxyCallFrame *frame = f_callstack_peek(&proc->call_stack); \
        if (!vm->running || !frame || !frame->func || frame->func->type != FOXY_FUNCTION_USER) \
            goto lbl_FOXCODE_HALT; \
        const FoxInstruction *code_end = frame->func->as.user.code + (frame->func->as.user.code_size / sizeof(FoxInstruction)); \
        if (!frame->ip || frame->ip >= code_end) \
            goto lbl_FOXCODE_HALT; \
        inst = *frame->ip++; \
        goto *dispatch_table[GET_FOXCODE(inst)]; \
    } while(0)

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
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }

        FoxyValue path_val = vm->constants[const_idx];
        char module_path[FOXY_MAX_MODULE_NAME_SIZE];
        module_path[0] = '\0';

        if (path_val.type == FOXY_VAL_ARRAY || path_val.type == FOXY_VAL_OBJECT) {
            const char *raw_data = f_value_get_char_array_data(&path_val);
            if (raw_data) {
                snprintf(module_path, sizeof(module_path), "%s", raw_data);
                free((void*)raw_data);
            }
        } else if (path_val.type == FOXY_VAL_CHAR) {
            const char *src = path_val.as.sval ? path_val.as.sval : path_val.as.string;
            if (src) {
                snprintf(module_path, sizeof(module_path), "%s", src);
            }
        }

        if (module_path[0] != '\0') {
            char lib_file[FOXY_MAX_MODULE_NAME_SIZE + 4]; 
            snprintf(lib_file, sizeof(lib_file), "%s.so", module_path);

            FoxyLib current_lib;
            memset(&current_lib, 0, sizeof(current_lib));
            snprintf(current_lib.path, sizeof(current_lib.path), "%s", module_path);

            f_vm_set_current_loading_lib(vm, &current_lib);

            void *handle = dlopen(lib_file, RTLD_NOW | RTLD_LOCAL);
            if (!handle) {
                fprintf(stderr, "[Foxy VM Error] No se pudo cargar el módulo '%s': %s\n", lib_file, dlerror());
                f_vm_set_current_loading_lib(vm, NULL);
                DISPATCH();
            }

            current_lib.handle = handle;
            dlerror();

            void (*foxy_init_module)(FoxyVM *) = (void (*)(FoxyVM *))dlsym(handle, "foxy_init_module");
            char *error = dlerror();
            if (error != NULL) {
                fprintf(stderr, "[Foxy VM Error] Símbolo 'foxy_init_module' no encontrado en %s: %s\n", lib_file, error);
                dlclose(handle);
                f_vm_set_current_loading_lib(vm, NULL);
                DISPATCH();
            }

            foxy_init_module(vm);
            f_vm_set_current_loading_lib(vm, NULL);
        }

        DISPATCH();
    }

    lbl_FOXCODE_COLLECT: {
        f_gc_collect();
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_CONST: {
        int const_idx = GETARG_Bx(inst);
        if (const_idx < (int)vm->constants_count) {
            f_vm_push(proc, vm->constants[const_idx]);
        } else {
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_NULL: {
        f_vm_push(proc, FOXY_NULL_VALUE);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_TRUE: {
        FoxyValue true_val = {0};
        true_val.type = FOXY_VAL_BOOL;
        true_val.as.boolean = true;
        f_vm_push(proc, true_val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_FALSE: {
        FoxyValue false_val = {0};
        false_val.type = FOXY_VAL_BOOL;
        false_val.as.boolean = false;
        f_vm_push(proc, false_val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_LOCAL: {
        int local_idx = GETARG_A(inst);
        if ((size_t)local_idx < proc->locals_capacity) {
            f_vm_push(proc, proc->locals[local_idx]);
        } else {
            vm->running = false;
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_STORE_LOCAL: {
        size_t local_idx = (size_t)GETARG_A(inst);
        FoxyValue val = f_vm_pop(proc);

        if (local_idx >= proc->locals_capacity) {
            size_t old_cap = proc->locals_capacity;
            size_t new_cap = old_cap == 0 ? FOXY_MAX_LOCALS_CAPACITY : old_cap * 2;
            while (local_idx >= new_cap) new_cap *= 2;
            
            FoxyValue *new_locals = (FoxyValue *)realloc(proc->locals, sizeof(FoxyValue) * new_cap);
            if (!new_locals) {
                proc->running = 0;
                proc->state = FOXY_PROCESS_DEAD;
                goto lbl_FOXCODE_HALT;
            }
            proc->locals = new_locals;
            proc->locals_capacity = new_cap;
        }

        proc->locals[local_idx] = val;
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_GLOBAL: {
        int global_idx = GETARG_Bx(inst);
        if (global_idx < 0 || global_idx >= (int)vm->constants_count) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        FoxyValue constant_val = vm->constants[global_idx];
        char *sym_name = NULL;
        bool allocated = false;

        if (constant_val.type == FOXY_VAL_ARRAY || constant_val.type == FOXY_VAL_OBJECT) {
            sym_name = (char *)f_value_get_char_array_data(&constant_val);
            allocated = true;
        } else {
            sym_name = (char *)constant_val.as.sval;
        }

        FoxyValue result_val = { .type = FOXY_VAL_NULL };

        if (sym_name && sym_name[0] != '\0') {
            if (vm->symtable) {
                FoxySymbolRow *row = f_symtable_find_by_name(vm->symtable, sym_name);
                if (row) result_val = row->value;
            }
            if (result_val.type == FOXY_VAL_NULL) {
                FoxyNativeMethod native_fn = f_vm_find_native(vm, sym_name);
                if (native_fn) {
                    result_val.type = FOXY_VAL_FUNCTION;
                    result_val.as.native_fn = (void *)native_fn;
                }
            }
        }

        if (allocated && sym_name) {
            free((void *)sym_name); // Cast a void* para corregir -Wdiscarded-qualifiers
        }

        f_vm_push(proc, result_val);
        DISPATCH();
    }
    
    lbl_FOXCODE_STORE_GLOBAL: {
        int global_idx = GETARG_Bx(inst);
        FoxyValue val = f_vm_pop(proc);

        if ((size_t)global_idx >= vm->globals_capacity) {
            size_t new_cap = vm->globals_capacity == 0 ? FOXY_MAX_LOCALS_CAPACITY : vm->globals_capacity * 2;
            while ((size_t)global_idx >= new_cap) new_cap *= 2;
            FoxyValue *new_globals = realloc(vm->globals, sizeof(FoxyValue) * new_cap);
            if (!new_globals) {
                vm->running = false;
                goto lbl_FOXCODE_HALT;
            }
            vm->globals = new_globals;
            vm->globals_capacity = new_cap;
        }

        vm->globals[global_idx] = val;
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_LIB: {
        int lib_name_idx = GETARG_Bx(inst);
        char *lib_name = NULL;
        bool allocated = false;

        if (lib_name_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[lib_name_idx];
            if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_OBJECT) {
                lib_name = (char *)f_value_get_char_array_data(&c);
                allocated = true;
            }
        }

        if (lib_name) {
            f_vm_load_library(vm, lib_name);
            if (allocated) free(lib_name);
        }
        DISPATCH();
    }
    
    lbl_FOXCODE_GET_MEMBER_PTR:
    lbl_FOXCODE_GET_MEMBER: {
        int member_idx = GETARG_Bx(inst);
        const char *member_name = NULL;

        if (member_idx >= 0 && member_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[member_idx];
            if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_OBJECT) {
                member_name = f_value_get_char_array_data(&c);
            } else if (c.as.sval != NULL) {
                member_name = c.as.sval;
            }
        }

        FoxyValue target = f_vm_pop(proc);
        FoxyValue result = {0};
        result.type = FOXY_VAL_NULL;

        if (target.type == FOXY_VAL_DICT && target.as.dict) {
            f_dict_get(target.as.dict, member_name, &result);
        } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
            if (!f_object_get_field(target.as.obj, member_name, &result) && target.as.obj->klass) {
                FoxyMethod *method = f_class_find_method(target.as.obj->klass, member_name);
                if (method) {
                    result.type = FOXY_VAL_FUNCTION;
                    result.as.ptr = method;
                }
            }
        }

        f_vm_push(proc, result);
        DISPATCH();
    }

    lbl_FOXCODE_SET_MEMBER_PTR:
    lbl_FOXCODE_SET_MEMBER: {
        int member_idx = GETARG_Bx(inst);
        const char *member_name = NULL;

        if (member_idx >= 0 && member_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[member_idx];
            if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_OBJECT) {
                member_name = f_value_get_char_array_data(&c);
            } else if (c.as.sval != NULL) {
                member_name = c.as.sval;
            }
        }

        FoxyValue val_to_assign = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);

        if (target.type == FOXY_VAL_DICT && target.as.dict) {
            f_dict_set(target.as.dict, member_name, val_to_assign);
        } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
            f_object_set_field(target.as.obj, member_name, val_to_assign);
        }

        DISPATCH();
    }

    lbl_FOXCODE_GET_INDEX: {
        FoxyValue index = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_NULL };

        if (target.type == FOXY_VAL_ARRAY && target.as.array) {
            if (index.type == FOXY_VAL_INT && index.as.ival >= 0 && (size_t)index.as.ival < target.as.array->length) {
                res = target.as.array->items[index.as.ival];
            }
        } else if (target.type == FOXY_VAL_DICT && target.as.dict) {
            const char *key = (index.type == FOXY_VAL_ARRAY || index.type == FOXY_VAL_OBJECT) ? f_value_get_char_array_data(&index) : index.as.sval;
            if (key) f_dict_get(target.as.dict, key, &res);
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_SET_INDEX: {
        FoxyValue val = f_vm_pop(proc);
        FoxyValue index = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);

        if (target.type == FOXY_VAL_ARRAY && target.as.array) {
            if (index.type == FOXY_VAL_INT && index.as.ival >= 0 && (size_t)index.as.ival < target.as.array->length) {
                target.as.array->items[index.as.ival] = val;
            }
        } else if (target.type == FOXY_VAL_DICT && target.as.dict) {
            const char *key = (index.type == FOXY_VAL_ARRAY || index.type == FOXY_VAL_OBJECT) ? f_value_get_char_array_data(&index) : index.as.sval;
            if (key) f_dict_set(target.as.dict, key, val);
        }

        DISPATCH();
    }

    lbl_FOXCODE_ADD: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_ADD, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_SUB: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_SUB, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_MUL: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_MUL, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_DIV: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_DIV, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_MOD: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_MOD, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NEG: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (a.type == FOXY_VAL_INT) {
            res.type = FOXY_VAL_INT;
            res.as.ival = -a.as.ival;
        } else if (a.type == FOXY_VAL_DOUBLE) {
            res.type = FOXY_VAL_DOUBLE;
            res.as.dval = -a.as.dval;
        } else {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NOT: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = false };
        if (a.type == FOXY_VAL_BOOL) {
            res.as.boolean = !a.as.boolean;
        } else if (a.type == FOXY_VAL_INT) {
            res.as.boolean = (a.as.ival == 0);
        } else if (a.type == FOXY_VAL_NULL) {
            res.as.boolean = true;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_EQ: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = f_value_equals(&a, &b) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NEQ: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = !f_value_equals(&a, &b) };
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_LT: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_LT, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_GT: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_GT, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_LE: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_LE, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_GE: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_binary_op(FOXCODE_GE, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_AND: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_bitwise_op(FOXCODE_BIT_AND, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_OR: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_bitwise_op(FOXCODE_BIT_OR, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_XOR: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_bitwise_op(FOXCODE_BIT_XOR, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_SHL: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_bitwise_op(FOXCODE_BIT_SHL, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_SHR: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_bitwise_op(FOXCODE_BIT_SHR, a, b, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_NOT: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = {0};
        if (!f_vm_eval_unary_bitwise_op(FOXCODE_BIT_NOT, a, &res)) {
            proc->running = 0;
            proc->state = FOXY_PROCESS_DEAD;
            goto lbl_FOXCODE_HALT;
        }
        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_FOR_ITER: {
        FoxyValue collection = f_vm_peek(proc, 0);
        if (collection.type != FOXY_VAL_ARRAY && collection.type != FOXY_VAL_DICT) {
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
            FoxyCallFrame *frame = f_callstack_peek(&proc->call_stack);
            if (frame && frame->func) {
                frame->ip = frame->func->as.user.code + exit_offset;
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_FOREACH_CALL: {
        DISPATCH();
    }

    lbl_FOXCODE_JUMP: {
        size_t target_offset = (size_t)GETARG_Bx(inst);
        FoxyCallFrame *frame = f_callstack_peek(&proc->call_stack);
        if (frame && frame->func) {
            // Asignación directa por aritmética de punteros sin índices numéricos
            frame->ip = frame->func->as.user.code + target_offset;
        }
        DISPATCH();
    }

    lbl_FOXCODE_JUMP_IF_FALSE: {
        FoxyCallFrame *frame = f_callstack_peek(&proc->call_stack);
        size_t target_offset = (size_t)GETARG_Bx(inst);
        FoxyValue condition = f_vm_pop(proc);
        if (!condition.as.boolean && frame && frame->func) {
            frame->ip = frame->func->as.user.code + target_offset;
        }
        DISPATCH();
    }

    lbl_FOXCODE_POP: {
        f_vm_pop(proc);
        DISPATCH();
    }

    lbl_FOXCODE_CALL: {
        int arg_count = GETARG_A(inst);
        FoxyValue callee_val = f_vm_pop(proc);
        
        if (callee_val.type == FOXY_VAL_FUNCTION) {
            FoxyFunction *func = callee_val.as.func;
            
            if (func->type == FOXY_FUNCTION_NATIVE) {
                if (func->as.native.function_ptr) {
                    func->as.native.function_ptr(vm, proc, arg_count);
                }
            } 
            else if (func->type == FOXY_FUNCTION_USER) {
                if (arg_count != func->arity) {
                    vm->running = false;
                    goto lbl_FOXCODE_HALT;
                }

                for (int i = arg_count - 1; i >= 0; i--) {
                    proc->locals[i] = f_vm_pop(proc);
                }

                const FoxInstruction *entry_ip = func->as.user.code;
                if (!f_callstack_push(&proc->call_stack, func, entry_ip, 0)) {
                    vm->running = false;
                    goto lbl_FOXCODE_HALT;
                }
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_RET: {
        if (proc->stack_top == 0) f_vm_push(proc, FOXY_NULL_VALUE);
        
        f_callstack_pop(&proc->call_stack);
        if (proc->call_stack.count == 0) {
            proc->state = FOXY_PROCESS_READY;
            vm->running = false;
            return FOXY_STATUS_SUCCESS;
        }
        DISPATCH();
    }

    lbl_FOXCODE_NEW_ARRAY: {
        int count = GETARG_A(inst);
        FoxyArray *arr = f_array_new((size_t)count, FOXY_VAL_NULL);
        if (arr) {
            for (int i = count - 1; i >= 0; i--) {
                arr->items[i] = f_vm_pop(proc);
            }
            arr->count = count;
        }
        FoxyValue arr_val = {0};
        arr_val.type = FOXY_VAL_ARRAY;
        arr_val.as.array = arr;
        f_vm_push(proc, arr_val);
        DISPATCH();
    }

    lbl_FOXCODE_NEW_DICT: {
        FoxyDict *dict = f_dict_new();
        FoxyValue dict_val = {0};
        dict_val.type = FOXY_VAL_DICT;
        dict_val.as.dict = dict;
        f_vm_push(proc, dict_val);
        DISPATCH();
    }

    lbl_FOXCODE_CLASS_NEW: {
        int name_idx = GETARG_Bx(inst);
        const char *class_name = NULL;
        if (name_idx >= 0 && name_idx < (int)vm->constants_count) {
            FoxyValue c = vm->constants[name_idx];
            if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_OBJECT) {
                class_name = f_value_get_char_array_data(&c);
            }
        }
        FoxyClass *klass = f_class_new(class_name, NULL);
        FoxyValue klass_val = {0};
        klass_val.type = FOXY_VAL_OBJECT;
        klass_val.as.ptr = klass;
        f_vm_push(proc, klass_val);
        DISPATCH();
    }

    lbl_FOXCODE_NEW_INSTANCE: {
        FoxyValue klass_val = f_vm_pop(proc);
        if (klass_val.type == FOXY_VAL_OBJECT && klass_val.as.ptr) {
            FoxyClass *klass = (FoxyClass *)klass_val.as.ptr;
            FoxyObject *obj = f_object_new(klass);
            FoxyValue obj_val = {0};
            obj_val.type = FOXY_VAL_OBJECT;
            obj_val.as.obj = obj;
            f_vm_push(proc, obj_val);
        } else {
            goto lbl_FOXCODE_HALT;
        }
        DISPATCH();
    }

    lbl_FOXCODE_METHOD_BIND: {
        FoxyValue fn_val = f_vm_pop(proc);
        FoxyValue klass_val = f_vm_pop(proc);
        int name_idx = GETARG_Bx(inst);

        if (klass_val.type == FOXY_VAL_OBJECT && klass_val.as.ptr && fn_val.type == FOXY_VAL_FUNCTION) {
            const char *method_name = NULL;
            if (name_idx >= 0 && name_idx < (int)vm->constants_count) {
                FoxyValue c = vm->constants[name_idx];
                if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_OBJECT) {
                    method_name = f_value_get_char_array_data(&c);
                }
            }
            if (method_name && fn_val.as.func->type == FOXY_FUNCTION_NATIVE) {
                f_class_add_method_native((FoxyClass *)klass_val.as.ptr, method_name, (FoxyMethodFunc)fn_val.as.func->as.native.function_ptr);
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_POPEN: {
        FoxyValue env_val  = f_vm_pop(proc);
        FoxyValue name_val = f_vm_pop(proc);
        FoxyValue fn_val   = f_vm_pop(proc);

        FoxyProtocol *protocol = (env_val.type == FOXY_VAL_OBJECT) ? (FoxyProtocol *)env_val.as.ptr : NULL;
        const char *pname = (name_val.type == FOXY_VAL_ARRAY || name_val.type == FOXY_VAL_OBJECT) ? f_value_get_char_array_data(&name_val) : "main_subproc";

        if (fn_val.type != FOXY_VAL_FUNCTION || !fn_val.as.func || fn_val.as.func->type != FOXY_FUNCTION_USER) {
            return FOXY_STATUS_RUNTIME;
        }

        FoxyFunction *func = fn_val.as.func;
        FoxyProcess *sub_proc = f_process_create(vm->runtime, pname, func, protocol);
        if (!sub_proc || !f_process_start(sub_proc)) {
            return FOXY_STATUS_RUNTIME;
        }

        FoxyValue proc_obj = {0};
        proc_obj.type = FOXY_VAL_OBJECT;
        proc_obj.as.ptr = sub_proc;
        f_vm_push(proc, proc_obj);  
        DISPATCH();
    }

    lbl_FOXCODE_ENV: {
        FoxyValue env_val = {0};
        env_val.type = FOXY_VAL_NULL;
        env_val.as.ptr = NULL;
        f_vm_push(proc, env_val);
        DISPATCH();
    }

    lbl_FOXCODE_ENV_CREATE: {
        FoxyValue env_name_val = f_vm_pop(proc);
        const char *env_name = (env_name_val.type == FOXY_VAL_ARRAY || env_name_val.type == FOXY_VAL_OBJECT) ? f_value_get_char_array_data(&env_name_val) : NULL;

        FoxyProtocol *protocol = f_runtime_get_or_create(vm->runtime, env_name);
        if (!protocol) return FOXY_STATUS_RUNTIME;

        FoxyValue prot_val = {0};
        prot_val.type = FOXY_VAL_OBJECT;
        prot_val.as.ptr = protocol;
        f_vm_push(proc, prot_val);
        DISPATCH();
    }

    lbl_FOXCODE_ENV_BIND: {
        FoxyValue env_val  = f_vm_pop(proc);
        FoxyValue proc_val = f_vm_pop(proc);

        if (env_val.type == FOXY_VAL_OBJECT && proc_val.type == FOXY_VAL_OBJECT) {
            FoxyProcess *target_proc = (FoxyProcess *)proc_val.as.ptr;
            FoxyProtocol *protocol   = (FoxyProtocol *)env_val.as.ptr;
            if (target_proc && protocol) target_proc->protocol = protocol;
        }

        f_vm_push(proc, proc_val);
        DISPATCH();
    }
}

// ==========================================
// INTERFACES PÚBLICAS DE LA VM
// ==========================================
FoxyStatus f_vm_run(FoxyVM *vm) {
    if (!vm || !vm->processes_hash) return FOXY_STATUS_SUCCESS;
    FoxyProcess *proc = vm->processes_hash;
    if (!proc) return FOXY_STATUS_RUNTIME;
    return f_vm_execute_process(vm, proc);
}

FoxyVM* f_vm_new(void) {
    FoxyVM *vm = calloc(1, sizeof(FoxyVM));
    if (!vm) return NULL;

    vm->constants = NULL;
    vm->constants_count = 0;
    vm->constants_capacity = 0;

    vm->native_symbols_hash = NULL;
    vm->processes_hash = NULL;

    vm->loaded_libs = NULL;
    vm->loaded_libs_count = 0;
    vm->loaded_libs_capacity = 0;

    vm->runtime = f_runtime_new();
    if (!vm->runtime) {
        free(vm);
        return NULL;
    }
    
    vm->running = false;
    vm->symtable = f_symtable_new();
    vm->loading_lib = NULL;

    if (!vm->symtable) {
        f_runtime_free(vm->runtime);
        free(vm);
        return NULL;
    }

    return vm;
}

FoxyNativeMethod f_vm_lookup_native(FoxyVM *vm, const char *name) {
    if (!vm || !name) return NULL;
    
    FoxyNativeSymbolEntry *entry = NULL;
    HASH_FIND_STR(vm->native_symbols_hash, name, entry);
    
    return entry ? entry->func : NULL;
}

void f_vm_free(FoxyVM *vm) {
    if (!vm) return;

    // 1. Liberar símbolos nativos (uthash)
    FoxyNativeSymbolEntry *curr_sym, *tmp_sym;
    HASH_ITER(hh, vm->native_symbols_hash, curr_sym, tmp_sym) {
        HASH_DEL(vm->native_symbols_hash, curr_sym);
        free(curr_sym);
    }

    // 2. Liberar procesos y su callstack
    FoxyProcess *curr_proc, *tmp_proc;
    HASH_ITER(hh, vm->processes_hash, curr_proc, tmp_proc) {
        HASH_DEL(vm->processes_hash, curr_proc);
        f_process_free(curr_proc);
    }

    // 3. Pool de constantes
    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            f_value_free_contents(&vm->constants[i]);
        }
        free(vm->constants);
        vm->constants = NULL;
    }

    // 4. Variables globales
    if (vm->globals) {
        free(vm->globals);
        vm->globals = NULL;
    }

    // 5. Runtime y Symtable
    if (vm->runtime) {
        f_runtime_free(vm->runtime);
        vm->runtime = NULL;
    }
    if (vm->symtable) {
        f_symtable_free(vm->symtable);
        vm->symtable = NULL;
    }

    // 6. Arreglo de librerías cargadas
    if (vm->loaded_libs) {
        for (size_t i = 0; i < vm->loaded_libs_count; i++) {
            free(vm->loaded_libs[i]);
        }
        free(vm->loaded_libs);
    }

    free(vm);
}