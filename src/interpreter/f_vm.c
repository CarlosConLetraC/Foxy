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
        f_process_free(proc, vm);
        return;
    }

    f_callstack_init(&proc->call_stack);

    FoxyFunction *main_func = (FoxyFunction *)calloc(1, sizeof(FoxyFunction));
    if (!main_func) {
        f_process_free(proc, vm);
        return;
    }
    main_func->type = FOXY_FUNCTION_USER;
    main_func->as.user.code = (FoxInstruction *)malloc(code_size);
    if (!main_func->as.user.code) {
        free(main_func);
        f_process_free(proc, vm);
        return;
    }
    memcpy(main_func->as.user.code, code, code_size);
    
    // Corregido: número de instrucciones en base al tamaño en bytes de FoxInstruction
    main_func->as.user.code_size = code_size / sizeof(FoxInstruction);

    // =========================================================================
    // VINCULACIÓN DEL POOL DE CONSTANTES (SOLUCIÓN)
    // Asignamos las constantes globales del VM cargadas por el parser al main_func
    // =========================================================================
    main_func->as.user.constants = vm->constants;
    main_func->as.user.constants_count = vm->constants_count;

    proc->main_func = main_func;
    if (!f_callstack_push(&proc->call_stack, main_func, main_func->as.user.code, 0)) {
        f_function_free(main_func, vm);
        f_process_free(proc, vm);
        return;
    }

    f_vm_add_process(vm, proc);
}

void f_vm_load_script_to_process(FoxyVM *vm, const uint8_t *blob, size_t code_size, const char *proc_name, const char *str_val) {
    if (str_val) {
        // En lugar de manipular vm->constants manualmente o realizar castings de punteros,
        // delegamos la construcción e inyección del arreglo al codegen.
        f_codegen_add_char_array_constant(vm, str_val);
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
        exit(1);
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

/* Tablas de búsqueda e inlining estáticos */
static const char * const FOXCODE_NAMES[FOXCODE_COUNT] = {
    #define F(fcode, name, symbol) [fcode] = name,
    FOXY_FOXCODE_LIST(F)
    #undef F
};

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

static inline const char *f_vm_foxcode_to_name(FOXY_FOXCODE fcode) {
    if (fcode >= FOXCODE_COUNT) return "UNKNOWN";
    const char *name = FOXCODE_NAMES[fcode];
    return name ? name : "UNKNOWN";
}

/* Macros para depuración dinámica en la VM */
#define CURRENT_OPCODE()       ((FOXY_FOXCODE)GET_FOXCODE(inst))
#define CURRENT_OP_NAME()      f_vm_foxcode_to_name(CURRENT_OPCODE())
#define CURRENT_OP_SYMBOL()    f_vm_foxcode_to_symbol(CURRENT_OPCODE())

#define VM_LOG(fmt, ...) \
    printf("          └─ [%s | '%s'] " fmt "\n", CURRENT_OP_NAME(), CURRENT_OP_SYMBOL(), ##__VA_ARGS__)

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

static bool f_vm_eval_unary_op(FOXY_FOXCODE code, FoxyValue input, FoxyValue *out) {
    if (code == FOXCODE_NEG) {
        switch (input.type) {
            case FOXY_VAL_INT:
                out->type = FOXY_VAL_INT;
                out->as.ival = -input.as.ival;
                return true;
            case FOXY_VAL_FLOAT:
                out->type = FOXY_VAL_FLOAT;
                out->as.fval = -input.as.fval;
                return true;
            default:
                return false; // Error de tipo
        }
    } 

    if (code == FOXCODE_NOT) {
        out->type = FOXY_VAL_BOOL;
        switch (input.type) {
            case FOXY_VAL_BOOL:
                out->as.boolean = !input.as.boolean;
                return true;
            case FOXY_VAL_NULL:
                out->as.boolean = true; // !null -> true
                return true;
            case FOXY_VAL_INT:
                out->as.boolean = (input.as.ival == 0); // !0 -> true, !n -> false
                return true;
            case FOXY_VAL_FLOAT:
                out->as.boolean = (input.as.fval == 0.0);
                return true;
            default:
                // Cualquier objeto, string o puntero válido se evalúa como truthy -> !obj = false
                out->as.boolean = false;
                return true;
        }
    }

    return false;
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

    FoxyCallFrame *frame = f_callstack_peek(&proc->call_stack);
    if (!frame) {
        proc->state = FOXY_PROCESS_DEAD;
        vm->running = false;
        return FOXY_STATUS_RUNTIME;
    }

    FoxyFunction *current_func = frame->func;
    const FoxInstruction *ip = frame->ip;
    size_t stack_base = frame->stack_base;

    #define DISPATCH() do { \
        inst = *ip++; \
        size_t ip_offset = (size_t)(frame->ip - frame->func->as.user.code - 1); \
        if (current_func && current_func->type == FOXY_FUNCTION_USER && ip >= current_func->as.user.code) \
            ip_offset = (size_t)(ip - current_func->as.user.code); \
        printf("[VM TRACE] IP:%04zu | Opcode: %-16s | Sym: %-5s | Hex: 0x%08X (A:%d, Bx:%d)\n", \
               ip_offset, CURRENT_OP_NAME(), CURRENT_OP_SYMBOL(), inst, GETARG_A(inst), GETARG_Bx(inst)); \
        goto *dispatch_table[GET_FOXCODE(inst)]; \
    } while (0)

    FoxInstruction inst;
    DISPATCH();

    lbl_FOXCODE_NOP: {
        DISPATCH();
    }

    lbl_FOXCODE_HALT: {
        FoxyCallFrame *popped_frame = f_callstack_pop(&proc->call_stack);
        (void)popped_frame;

        // Si aún hay marcos en la pila de llamadas, regresamos a la función que la invocó
        if (proc->call_stack.count > 0) {
            // Asegurar que si la función no dejó un retorno explícito, empuje NULL
            // para que instrucciones como STORE_LOCAL puedan consumirlo
            if (proc->stack_top == popped_frame->stack_base) f_vm_push(proc, FOXY_NULL_VALUE);
            frame = f_callstack_peek(&proc->call_stack);
            current_func = frame->func;
            ip = frame->ip;
            stack_base = frame->stack_base;
            DISPATCH();
        }

        // Fin del proceso principal (Top-Level)
        proc->state = FOXY_PROCESS_DEAD;
        vm->running = false;
        return FOXY_STATUS_SUCCESS;
    }

    lbl_FOXCODE_INCLUDE: {
        uint16_t const_idx = GETARG_Bx(inst);
        if (current_func && current_func->type == FOXY_FUNCTION_USER && const_idx < current_func->as.user.constants_count) {
            FoxyValue path_val = current_func->as.user.constants[const_idx];
            const char *mod_path = f_value_get_char_array_data(&path_val);
            if (mod_path) f_vm_load_module(vm, mod_path);
        }
        DISPATCH();
    }

    lbl_FOXCODE_COLLECT: {
        VM_LOG("Ejecutando recolección de basura (GC)");
        f_gc_collect();
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_CONST: {
        uint16_t const_idx = GETARG_Bx(inst);
        
        FoxyValue *pool = NULL;
        size_t pool_count = 0;

        // Intentar leer de la función actual
        if (current_func && current_func->type == FOXY_FUNCTION_USER && current_func->as.user.constants) {
            pool = current_func->as.user.constants;
            pool_count = current_func->as.user.constants_count;
        } else if (vm && vm->constants) { // Fallback al pool del VM
            pool = vm->constants;
            pool_count = vm->constants_count;
        }

        if (pool && const_idx < pool_count) {
            printf("pool-type: %d\n", pool[const_idx].type);
            f_vm_push(proc, pool[const_idx]);
        } else {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                "Índice de constante fuera de rango (%u)", const_idx);
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_NULL: {
        f_vm_push(proc, FOXY_NULL_VALUE);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_TRUE: {
        VM_LOG("Carga de valor TRUE en stack");
        FoxyValue true_val = {0};
        true_val.type = FOXY_VAL_BOOL;
        true_val.as.boolean = true;
        f_vm_push(proc, true_val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_FALSE: {
        VM_LOG("Carga de valor FALSE en stack");
        FoxyValue false_val = {0};
        false_val.type = FOXY_VAL_BOOL;
        false_val.as.boolean = false;
        f_vm_push(proc, false_val);
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_LOCAL: {
        uint8_t local_idx = GETARG_A(inst);
        size_t abs_slot = stack_base + local_idx;

        // Se valida contra la capacidad reservada o el conteo actual de locales válidos
        if (abs_slot < proc->locals_count) {
            f_vm_push(proc, proc->locals[abs_slot]);
        } else {
            f_vm_push(proc, FOXY_NULL_VALUE);
        }
        DISPATCH();
    }

    lbl_FOXCODE_STORE_LOCAL: {
        uint8_t local_idx = GETARG_A(inst);
        size_t abs_slot = stack_base + local_idx;
        FoxyValue val = f_vm_pop(proc);

        if (abs_slot >= proc->locals_capacity) {
            size_t new_cap = proc->locals_capacity == 0 ? FOXY_MAX_LOCALS_CAPACITY : proc->locals_capacity * 2;
            while (new_cap <= abs_slot) new_cap *= 2;

            FoxyValue *new_locals = (FoxyValue *)realloc(proc->locals, sizeof(FoxyValue) * new_cap);
            if (!new_locals) {
                f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_NOT_ENOUGH_MEMORY, "Falta de memoria asignando variables locales");
                proc->state = FOXY_PROCESS_DEAD;
                vm->running = false;
                return FOXY_STATUS_MEMORY;
            }

            // Inicializar nuevas posiciones vacías con FOXY_NULL_VALUE para evitar garbage data
            for (size_t i = proc->locals_capacity; i < new_cap; i++) new_locals[i] = FOXY_NULL_VALUE;

            proc->locals = new_locals;
            proc->locals_capacity = new_cap;
        }

        proc->locals[abs_slot] = val;
        if (abs_slot >= proc->locals_count) {
            proc->locals_count = abs_slot + 1;
        }
        DISPATCH();
    }

    lbl_FOXCODE_LOAD_GLOBAL: {
        VM_LOG("FOXCODE_LOAD_GLOBAL");
        uint16_t const_idx = GETARG_Bx(inst);
        if (current_func && current_func->type == FOXY_FUNCTION_USER && const_idx < current_func->as.user.constants_count) {
            FoxyValue key_val = current_func->as.user.constants[const_idx];
            const char *var_name = f_value_get_char_array_data(&key_val);
            printf("nombre referencia global foxy: %s\n", var_name);
            FoxyValue resolved_val = FOXY_NULL_VALUE;
            if (var_name && vm->symtable) {
                FoxySymbolRow *row = f_symtable_find_by_name(vm->symtable, var_name);
                if (row) resolved_val = row->value;
            }
            f_vm_push(proc, resolved_val);
        } else {
            f_vm_push(proc, FOXY_NULL_VALUE);
        }
        DISPATCH();
    }
    
    lbl_FOXCODE_STORE_GLOBAL: {
        int global_idx = GETARG_Bx(inst);
        FoxyValue val = f_vm_pop(proc);
        VM_LOG("Asignando valor tipo [%s] a global idx [%d]", FOXY_VALUE_TYPE_NAMES[val.type], global_idx);

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
            VM_LOG("Cargando libreria externa: '%s'", lib_name);
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

        VM_LOG("Obteniendo miembro/propiedad: '%s'", member_name ? member_name : "NULL");

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

        VM_LOG("Asignando miembro/propiedad: '%s'", member_name ? member_name : "NULL");

        FoxyValue val_to_assign = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);

        if (target.type == FOXY_VAL_DICT && target.as.dict) {
            f_dict_set(target.as.dict, member_name, val_to_assign, vm);
        } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
            f_object_set_field(target.as.obj, member_name, val_to_assign, vm);
        }

        DISPATCH();
    }

    lbl_FOXCODE_GET_INDEX: {
        FoxyValue index = f_vm_pop(proc);
        FoxyValue target = f_vm_pop(proc);
        FoxyValue res = {0};
        res.type = FOXY_VAL_NULL;
        VM_LOG("Obteniendo elemento por indice");

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
        VM_LOG("Asignando elemento por indice");

        if (target.type == FOXY_VAL_ARRAY && target.as.array) {
            if (index.type == FOXY_VAL_INT && index.as.ival >= 0 && (size_t)index.as.ival < target.as.array->length) {
                target.as.array->items[index.as.ival] = val;
            }
        } else if (target.type == FOXY_VAL_DICT && target.as.dict) {
            const char *key = (index.type == FOXY_VAL_ARRAY || index.type == FOXY_VAL_OBJECT) ? f_value_get_char_array_data(&index) : index.as.sval;
            if (key) f_dict_set(target.as.dict, key, val, vm);
        }

        DISPATCH();
    }

    lbl_FOXCODE_ADD:
    lbl_FOXCODE_SUB:
    lbl_FOXCODE_MUL:
    lbl_FOXCODE_DIV:
    lbl_FOXCODE_MOD:
    lbl_FOXCODE_EQ:
    lbl_FOXCODE_NEQ:
    lbl_FOXCODE_LT:
    lbl_FOXCODE_GT:
    lbl_FOXCODE_LE:
    lbl_FOXCODE_GE: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = FOXY_NULL_VALUE;

        if (!f_vm_eval_binary_op((FOXY_FOXCODE)GET_FOXCODE(inst), a, b, &res)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_ARITHMETIC, "Operación aritmética o de comparación inválida");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NEG: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = FOXY_NULL_VALUE;

        // Inversión de signo numérico (-num ; -(10))
        if (!f_vm_eval_unary_op(FOXCODE_NEG, a, &res)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_ARITHMETIC, "Operando inválido para el operador unario '-'");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_NOT: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = FOXY_NULL_VALUE;

        // Inversión lógica (!valor)
        if (!f_vm_eval_unary_op(FOXCODE_NOT, a, &res)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_CAST, "Operando inválido para el operador lógico '!'");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_AND:
    lbl_FOXCODE_BIT_OR:
    lbl_FOXCODE_BIT_XOR:
    lbl_FOXCODE_BIT_SHL:
    lbl_FOXCODE_BIT_SHR: {
        FoxyValue b = f_vm_pop(proc);
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = FOXY_NULL_VALUE;

        if (!f_vm_eval_bitwise_op((FOXY_FOXCODE)GET_FOXCODE(inst), a, b, &res)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_BITWISE, "Operación a nivel de bits inválida");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_BIT_NOT: {
        FoxyValue a = f_vm_pop(proc);
        FoxyValue res = FOXY_NULL_VALUE;

        if (!f_vm_eval_unary_bitwise_op((FOXY_FOXCODE)GET_FOXCODE(inst), a, &res)) {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_BITWISE, "Operación NOT a nivel de bits inválida");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        f_vm_push(proc, res);
        DISPATCH();
    }

    lbl_FOXCODE_WHILE_ITER: {
        // Evaluación del bucle while a nivel de VM
        FoxyValue cond = f_vm_pop(proc);
        bool is_truthy = false;

        if (cond.type == FOXY_VAL_BOOL) {
            is_truthy = cond.as.boolean;
        } else if (f_value_is_numeric(&cond)) {
            is_truthy = (cond.as.ival != 0);
        }

        if (!is_truthy) {
            uint16_t exit_offset = GETARG_Bx(inst);
            if (current_func && current_func->type == FOXY_FUNCTION_USER) {
                ip = current_func->as.user.code + exit_offset;
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_FOR_ITER: {
        FoxyValue collection = f_vm_peek(proc, 0);
        if (collection.type != FOXY_VAL_ARRAY && collection.type != FOXY_VAL_DICT) {
            VM_LOG("Error: Coleccion no iterable en FOR_ITER");
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
            VM_LOG("Iteracion finalizada, saltando a offset %zu", exit_offset);
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
        uint16_t target_ip = GETARG_Bx(inst);
        if (current_func && current_func->type == FOXY_FUNCTION_USER) {
            ip = current_func->as.user.code + target_ip;
        }
        DISPATCH();
    }

    lbl_FOXCODE_JUMP_IF_FALSE: {
        uint16_t target_offset = GETARG_Bx(inst);
        FoxyValue condition = f_vm_pop(proc);

        bool is_truthy = true;
        if (condition.type == FOXY_VAL_BOOL) {
            is_truthy = condition.as.boolean;
        } else if (condition.type == FOXY_VAL_NULL) {
            is_truthy = false;
        } else if (f_value_is_numeric(&condition)) {
            is_truthy = (condition.as.ival != 0);
        }

        if (!is_truthy && current_func && current_func->type == FOXY_FUNCTION_USER) {
            ip = current_func->as.user.code + target_offset;
        }
        DISPATCH();
    }

    /*lbl_FOXCODE_POP: {
        uint8_t count = GETARG_A(inst);
        if (count == 0) count = 1;
        f_vm_stack_pop_n(vm, count);
        DISPATCH();
    }*/
    lbl_FOXCODE_POP: {
        uint8_t count = GETARG_A(inst);
        if (count == 0) count = 1;
        for (uint8_t i = 0; i < count; i++) {
            f_vm_pop(proc);
        }
        DISPATCH();
    }

    lbl_FOXCODE_CALL: {
        uint8_t arg_count = GETARG_A(inst);
        FoxyValue callee_val = f_vm_pop(proc);

        if (callee_val.type == FOXY_VAL_FUNCTION && callee_val.as.func) {
            FoxyFunction *func = callee_val.as.func;

            if (func->type == FOXY_FUNCTION_USER) {
                // Guardar el IP del frame actual antes del salto
                frame->ip = ip;

                if (proc->stack_top < arg_count) {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                        "Argumentos insuficientes en la pila para la llamada");
                    proc->state = FOXY_PROCESS_DEAD;
                    vm->running = false;
                    return FOXY_STATUS_RUNTIME;
                }

                size_t new_stack_base = proc->stack_top - arg_count;

                for (size_t i = 0; i < arg_count; i++) {
                    FoxyValue arg_val = proc->stack[new_stack_base + i];
                    size_t abs_slot = new_stack_base + i;

                    if (abs_slot >= proc->locals_capacity) {
                        size_t new_cap = proc->locals_capacity == 0 ? FOXY_MAX_LOCALS_CAPACITY : proc->locals_capacity * 2;
                        while (new_cap <= abs_slot) new_cap *= 2;
                        FoxyValue *new_locals = (FoxyValue *)realloc(proc->locals, sizeof(FoxyValue) * new_cap);
                        if (!new_locals) {
                            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, "Out of memory en variables locales");
                            proc->state = FOXY_PROCESS_DEAD;
                            vm->running = false;
                            return FOXY_STATUS_RUNTIME;
                        }
                        proc->locals = new_locals;
                        proc->locals_capacity = new_cap;
                    }
                    proc->locals[abs_slot] = arg_val;
                    if (abs_slot >= proc->locals_count) proc->locals_count = abs_slot + 1;
                }

                proc->stack_top = new_stack_base;

                if (!f_callstack_push(&proc->call_stack, func, func->as.user.code, new_stack_base)) {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, "Callstack Overflow");
                    proc->state = FOXY_PROCESS_DEAD;
                    vm->running = false;
                    return FOXY_STATUS_RUNTIME;
                }

                // Actualizar referencias activas
                frame = f_callstack_peek(&proc->call_stack);
                current_func = frame->func;
                ip = frame->ip;
                stack_base = frame->stack_base;
            } else if (func->type == FOXY_FUNCTION_NATIVE && func->as.native.function_ptr) {
                func->as.native.function_ptr(vm, proc, (int)arg_count);
            }
        } else {
            f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, "El objeto llamado no es una función invocable");
            proc->state = FOXY_PROCESS_DEAD;
            vm->running = false;
            return FOXY_STATUS_RUNTIME;
        }

        DISPATCH();
    }

    lbl_FOXCODE_RET: {
        VM_LOG("Retornando de llamada de función");
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
        VM_LOG("Creando nuevo arreglo con %d elementos", count);
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
        VM_LOG("Creando nuevo diccionario");
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
            if (c.type == FOXY_VAL_ARRAY || c.type == FOXY_VAL_CLASS) {
                class_name = f_value_get_char_array_data(&c);
            }
        }
        VM_LOG("Instanciando nueva clase: '%s'", class_name ? class_name : "Anonima");
        FoxyClass *klass = f_class_new(class_name, NULL);
        FoxyValue klass_val = {0};
        klass_val.type = FOXY_VAL_CLASS;
        klass_val.as.ptr = klass;
        f_vm_push(proc, klass_val);
        DISPATCH();
    }

    lbl_FOXCODE_NEW_OBJECT: {
        FoxyValue klass_val = f_vm_pop(proc);
        VM_LOG("Creando instancia de clase");
        if (klass_val.type == FOXY_VAL_CLASS && klass_val.as.ptr) {
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
            VM_LOG("Vinculando metodo '%s' a clase", method_name ? method_name : "NULL");
            if (method_name && fn_val.as.func->type == FOXY_FUNCTION_NATIVE) {
                f_class_add_method_native((FoxyClass *)klass_val.as.ptr, method_name, (FoxyMethodFunc)fn_val.as.func->as.native.function_ptr);
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_POPEN: {
        FoxyValue env_val      = f_vm_pop(proc);
        FoxyValue name_val     = f_vm_pop(proc);
        FoxyValue callback_val = f_vm_pop(proc);

        const char *pname = f_value_get_char_array_data(&name_val);
        FoxyFunction *fn = (callback_val.type == FOXY_VAL_FUNCTION) ? callback_val.as.func : NULL;
        FoxyProtocol *prot = (env_val.type == FOXY_VAL_OBJECT) ? (FoxyProtocol *)env_val.as.obj : NULL;

        if (pname && fn && vm->runtime) {
            FoxyProcess *new_proc = f_runtime_process_create(vm->runtime, pname, fn, prot);
            if (new_proc) {
                new_proc->vm = vm;
                f_runtime_process_start(new_proc);
            }
        }
        DISPATCH();
    }

    lbl_FOXCODE_ENV: {
        FoxyValue env_val = {0};
        env_val.type = FOXY_VAL_OBJECT;
        env_val.as.obj = (FoxyObject *)proc->protocol;
        f_vm_push(proc, env_val);
        DISPATCH();
    }

    lbl_FOXCODE_ENV_CREATE: {
        FoxyValue name_val = f_vm_pop(proc);
        const char *env_name = f_value_get_char_array_data(&name_val);

        if (env_name && vm->runtime) {
            FoxyProtocol *prot = f_runtime_get_or_create(vm->runtime, env_name);
            FoxyValue prot_val = {0};
            prot_val.type = FOXY_VAL_OBJECT;
            prot_val.as.obj = (FoxyObject *)prot;
            f_vm_push(proc, prot_val);
        } else {
            f_vm_push(proc, FOXY_NULL_VALUE);
        }
        DISPATCH();
    }

    lbl_FOXCODE_ENV_BIND: {
        FoxyValue env_val  = f_vm_pop(proc);
        FoxyValue proc_val = f_vm_pop(proc);

        if (env_val.type == FOXY_VAL_OBJECT && proc_val.type == FOXY_VAL_OBJECT) {
            FoxyProcess *target_proc = (FoxyProcess *)proc_val.as.obj;
            FoxyProtocol *target_env = (FoxyProtocol *)env_val.as.obj;
            if (target_proc) {
                target_proc->protocol = target_env;
            }
        }
        DISPATCH();
    }

    /*lbl_FOXCODE_PUSH_LIST:
    lbl_FOXCODE_PUSH_DICT:
    lbl_FOXCODE_SUPER_CALL:
    lbl_FOXCODE_THROW:
    lbl_FOXCODE_TRY_BEGIN:
    lbl_FOXCODE_TRY_END: {
        VM_LOG("Opcode extendido o reservado alcanzado: %s", CURRENT_OP_NAME());
        DISPATCH();
    }*/
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
        f_process_free(curr_proc, vm);
    }

    // 3. Pool de constantes
    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            f_value_free_contents(&vm->constants[i], vm);
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