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
#include "f_scope.h"
#include "f_runtime.h"

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
        // En foxy-lang las cadenas son representadas como objetos (FOXY_VAL_OBJECT) usando el puntero obj
        vm->constants[target_idx].type = FOXY_VAL_OBJECT;
        vm->constants[target_idx].as.obj = (FoxyObject *)str_val;
    }

    // f_vm_load_process devuelve void, se invoca directamente
    f_vm_load_process(vm, blob, code_size, proc_name);
}

FoxyValue f_vm_pop(FoxyProcess *p) {
    if (!p || p->stack_top == 0) {
        // Si tu proceso apunta al vm (ej: p->vm), lo pasamos para el contexto; de lo contrario usa NULL
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

// Elimina 'n' elementos del tope de la pila del proceso actual
void f_vm_stack_pop_n(FoxyVM* vm, size_t n) {
    if (!vm || vm->process_count == 0) return;
    FoxyProcess* proc = vm->processes[vm->current_process_index];
    if (proc->stack_top >= n) 
        proc->stack_top -= n;
    else
        proc->stack_top = 0;
}

FoxyNativeMethod f_vm_find_native(FoxyVM *vm, const char *name) {
    if (!vm || !name) return NULL;

    for (size_t i = 0; i < vm->native_symbols_count; i++) {
        if (strcmp(vm->native_symbols[i].name, name) == 0)
            return vm->native_symbols[i].func;
    }

    return NULL;
}

void f_vm_load_module(FoxyVM *vm, const char *raw_module_path) {
    if (!vm || !raw_module_path) return;

    // 1. Buffer para el nombre del módulo usando tu macro
    char module_path[FOXY_NAME_BUFFER_SIZE];
    size_t raw_len = strlen(raw_module_path);
    
    if (raw_len >= 2 && raw_module_path[0] == '"' && raw_module_path[raw_len - 1] == '"') {
        size_t clean_len = raw_len - 2;
        if (clean_len >= FOXY_NAME_BUFFER_SIZE) clean_len = FOXY_NAME_BUFFER_SIZE - 1;
        memcpy(module_path, raw_module_path + 1, clean_len);
        module_path[clean_len] = '\0';
    } else {
        strncpy(module_path, raw_module_path, FOXY_NAME_BUFFER_SIZE - 1);
        module_path[FOXY_NAME_BUFFER_SIZE - 1] = '\0';
    }

    // 2. Verificar si ya fue cargado previamente
    for (size_t i = 0; i < vm->loaded_libs_count; i++) {
        if (strcmp(vm->loaded_libs[i], module_path) == 0) {
            return;
        }
    }

    // 3. Buffer de la librería con espacio garantizado para el sufijo ".so"
    char lib_path[FOXY_NAME_BUFFER_SIZE + 8];
    snprintf(lib_path, sizeof(lib_path), "%s.so", module_path);

    // 4. Cargar con dlopen...
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "[Foxy VM Error] No se pudo cargar el módulo '%s' (%s): %s\n", module_path, lib_path, dlerror());
        return;
    }

    // 5. Buscar el símbolo estándar
    void (*init_module)(FoxyVM *) = (void (*)(FoxyVM *))dlsym(handle, "foxy_init_module");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "[Foxy VM Error] El módulo '%s' no exporta 'foxy_init_module': %s\n", module_path, error);
        dlclose(handle);
        return;
    }

    // 6. Ejecutar inicialización
    init_module(vm);

    // 7. Registrar en el VM
    if (vm->loaded_libs_count >= vm->loaded_libs_capacity) {
        size_t new_cap = vm->loaded_libs_capacity == 0 ? 8 : vm->loaded_libs_capacity * 2;
        char **new_libs = realloc(vm->loaded_libs, sizeof(char*) * new_cap);
        if (new_libs) {
            vm->loaded_libs = new_libs;
            vm->loaded_libs_capacity = new_cap;
        }
    }
    vm->loaded_libs[vm->loaded_libs_count++] = strdup(module_path);

    #ifdef FOXY_DEBUG_MODE
    printf("[DEBUG VM] Módulo dinámico cargado con éxito: '%s'\n", module_path);
    #endif
}

int f_vm_run(FoxyVM *vm) {
    if (!vm || vm->process_count == 0) return 0;

    vm->running = true;
    FoxyProcess *proc = vm->processes[vm->current_process_index];
    proc->state = FOXY_PROCESS_RUNNING;

    // Casteo directo al tipo de instrucción Foxy
    FoxInstruction *code = (FoxInstruction *)proc->bytecode;

    while (vm->running && proc->ip < (proc->bytecode_size / sizeof(FoxInstruction))) {
        FoxInstruction inst = code[proc->ip++];
        FOXY_FOXCODE foxcode = GET_FOXCODE(inst);

        switch (foxcode) {
            case FOXCODE_NOP:
                break;

            case FOXCODE_HALT:
                proc->running = 0;
                proc->state = FOXY_PROCESS_DEAD;
                vm->running = false;
                break;

            case FOXCODE_INCLUDE: {
                // Extracción inmediata del operando Bx (16 bits)
                int const_idx = GETARG_Bx(inst);

                if (const_idx >= (int)vm->constants_count) {
                    fprintf(stderr, "[Foxy VM Error] Índice de constante fuera de rango en FOXCODE_INCLUDE (PID %u)\n", proc->pid);
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
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
                break;
            }

            /* ==========================================
             * CONSTANTES Y LITERALES
             * ========================================== */
            case FOXCODE_LOAD_CONST: {
                int const_idx = GETARG_Bx(inst);
                // printf("[DEBUG VM] Cargando constante en índice [%d]\n", const_idx); // <-- Añade esto temporalmente
                if (const_idx < (int)vm->constants_count) {
                    f_vm_push(proc, vm->constants[const_idx]);
                } else {
                    fprintf(stderr, "[Foxy VM Error] Constant index out of bounds: %d\n", const_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_LOAD_NULL: {
                f_vm_push(proc, FOXY_NULL_VALUE);
                break;
            }

            case FOXCODE_LOAD_TRUE: {
                FoxyValue val = { .type = FOXY_VAL_BOOL, .as.boolean = true };
                f_vm_push(proc, val);
                break;
            }

            case FOXCODE_LOAD_FALSE: {
                FoxyValue val = { .type = FOXY_VAL_BOOL, .as.boolean = false };
                f_vm_push(proc, val);
                break;
            }

            /* ==========================================
             * VARIABLES Y SCOPES
             * ========================================== */
            case FOXCODE_LOAD_LOCAL: {
                int local_idx = GETARG_A(inst);
                if ((size_t)local_idx < proc->locals_capacity) {
                    // Si la posición está dentro de la capacidad pero aún no se asignó, empuja NULL o el valor actual
                    f_vm_push(proc, proc->locals[local_idx]);
                } else {
                    fprintf(stderr, "[Foxy VM Error] Local variable index out of bounds: %d\n", local_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_STORE_LOCAL: {
                size_t local_idx = (size_t)GETARG_A(inst);
                FoxyValue val = f_vm_pop(proc);

                if (local_idx >= proc->locals_capacity) {
                    size_t new_cap = proc->locals_capacity == 0 ? 16 : proc->locals_capacity * 2;
                    while (local_idx >= new_cap) new_cap *= 2;
                    
                    FoxyValue *new_locals = (FoxyValue *)realloc(proc->locals, sizeof(FoxyValue) * new_cap);
                    if (!new_locals) {
                        fprintf(stderr, "[Foxy VM Error] Out of memory allocating local variables\n");
                        vm->running = false;
                        break;
                    }
                    proc->locals = new_locals;
                    proc->locals_capacity = new_cap;
                }

                proc->locals[local_idx] = val;
                if (local_idx >= proc->locals_count) {
                    proc->locals_count = local_idx + 1;
                }
                break;
            }

            case FOXCODE_LOAD_GLOBAL: {
                int const_idx = GETARG_Bx(inst);
                
                if (const_idx >= (int)vm->constants_count) {
                    fprintf(stderr, "[Foxy VM Error] Índice de constante fuera de rango en LOAD_GLOBAL: %d\n", const_idx);
                    return -1;
                }

                FoxyValue constant_val = vm->constants[const_idx];
                const char *func_name = NULL; // <--- Declarada una sola vez como const char *

                if (constant_val.type == FOXY_VAL_ARRAY || constant_val.type == FOXY_VAL_OBJECT) {
                    func_name = f_value_get_char_array_data(&constant_val);
                } else if (constant_val.type == FOXY_VAL_CHAR) {
                    // CORREGIDO: Se asigna a la variable existente, sin redeclararla (sin 'const char *' enfrente)
                    func_name = constant_val.as.sval ? constant_val.as.sval : constant_val.as.string;
                }

                if (!func_name) {
                    fprintf(stderr, "[Foxy VM Error] Nombre de símbolo global inválido.\n");
                    return -1;
                }

                FoxyNativeMethod native_fn = f_vm_find_native(vm, func_name);
                FoxyValue fn_val;

                if (native_fn) {
                    fn_val.type = FOXY_VAL_FUNCTION;
                    fn_val.as.native_fn = (void *)native_fn;
                } else {
                    fn_val.type = FOXY_VAL_NULL;
                    fn_val.as.object = NULL;
                    fprintf(stderr, "[Foxy VM Warning] Símbolo no encontrado: '%s'\n", func_name);
                }

                f_vm_push(proc, fn_val);
                break;
            }

            case FOXCODE_STORE_GLOBAL: {
                int global_idx = GETARG_Bx(inst);
                FoxyValue val = f_vm_pop(proc);
                (void)val;
                (void)global_idx;
                break;
            }

            case FOXCODE_LOAD_LIB: {
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
                    }
                } else {
                    fprintf(stderr, "[Foxy VM Error] Constant index %d is not a valid CHAR array for FOXCODE_LOAD_LIB\n", lib_name_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_GET_MEMBER: {
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
                    break;
                }

                FoxyValue target = f_vm_pop(proc);
                FoxyValue result = { .type = FOXY_VAL_NULL };

                if (target.type == FOXY_VAL_DICT && target.as.dict) {
                    if (!f_dict_get(target.as.dict, member_name, &result)) {
                        fprintf(stderr, "[Foxy VM Error] La clave '%s' no existe en el diccionario\n", member_name);
                        proc->running = 0;
                        proc->state = FOXY_PROCESS_DEAD;
                        break;
                    }
                } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
                    if (!f_object_get_field(target.as.obj, member_name, &result)) {
                        fprintf(stderr, "[Foxy VM Error] El atributo '%s' no existe en el objeto\n", member_name);
                        proc->running = 0;
                        proc->state = FOXY_PROCESS_DEAD;
                        break;
                    }
                } else {
                    fprintf(stderr, "[Foxy VM Error] Intento de acceder al miembro '%s' en un tipo no válido (%s)\n",
                            member_name, f_value_type_to_string(target.type));
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }

                f_vm_push(proc, result);
                break;
            }

            case FOXCODE_SET_MEMBER: {
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
                    break;
                }

                FoxyValue val_to_assign = f_vm_pop(proc);
                FoxyValue target = f_vm_pop(proc);

                if (target.type == FOXY_VAL_DICT && target.as.dict) {
                    f_dict_set(target.as.dict, member_name, val_to_assign);
                } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
                    f_object_set_field(target.as.obj, member_name, val_to_assign);
                } else {
                    fprintf(stderr, "[Foxy VM Error] Intento de escribir el miembro '%s' en un tipo no mutable (%s)\n",
                            member_name, f_value_type_to_string(target.type));
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }
                break;
            }

            /* ==========================================
             * ARITMÉTICA Y LÓGICA
             * ========================================== */
            case FOXCODE_ADD: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival + b.as.ival };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_SUB: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival - b.as.ival };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_MUL: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival * b.as.ival };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_DIV: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                if (b.as.ival == 0) {
                    fprintf(stderr, "[Foxy VM Error] Division by zero\n");
                    vm->running = false;
                    break;
                }
                FoxyValue res = { .type = FOXY_VAL_INT, .as.ival = a.as.ival / b.as.ival };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_EQ: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival == b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_NEQ: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival != b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_LT: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival < b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_GT: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival > b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_LE: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival <= b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            case FOXCODE_GE: {
                FoxyValue b = f_vm_pop(proc);
                FoxyValue a = f_vm_pop(proc);
                FoxyValue res = { .type = FOXY_VAL_BOOL, .as.boolean = (a.as.ival >= b.as.ival) };
                f_vm_push(proc, res);
                break;
            }

            /* ==========================================
             * CONTROL DE CICLOS E ITERADORES
             * ========================================== */
            case FOXCODE_FOR_ITER: {
                FoxyValue collection = f_vm_peek(proc, 0);

                if (collection.type != FOXY_VAL_ARRAY && collection.type != FOXY_VAL_DICT) {
                    fprintf(stderr, "[Foxy VM Error] Intento de iterar sobre un tipo no válido (%s)\n",
                            f_value_type_to_string(collection.type));
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }

                FoxyValue iter_state = { .type = FOXY_VAL_INT, .as.ival = 0 };
                f_vm_push(proc, iter_state);
                break;
            }

            case FOXCODE_FOR_NEXT: {
                // El target IP viene directo en el campo Bx de la instrucción
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
                break;
            }

            case FOXCODE_FOREACH_CALL: {
                uint8_t callback_args_count = (uint8_t)GETARG_A(inst);
                FoxyValue callback = f_vm_pop(proc);
                (void)callback_args_count;
                (void)callback;
                break;
            }

            /* ==========================================
             * CONTROL DE FLUJO
             * ========================================== */
            case FOXCODE_JUMP: {
                size_t target_address = (size_t)GETARG_Bx(inst);
                proc->ip = target_address;
                break;
            }

            case FOXCODE_JUMP_IF_FALSE: {
                size_t target_address = (size_t)GETARG_Bx(inst);
                FoxyValue condition = f_vm_pop(proc);
                if (!condition.as.boolean) {
                    proc->ip = target_address;
                }
                break;
            }

            case FOXCODE_CALL: {
                int arg_count = GETARG_A(inst);
                
                // 1. Extraer el callee del stack (que ahora es un FoxyValue)
                FoxyValue callee_val = f_vm_pop(proc);
                
                // 2. Verificar si es un Array (tipo 10 / FOXY_VAL_ARRAY)
                if (callee_val.type == FOXY_VAL_ARRAY && callee_val.as.array != NULL) {
                    FoxyArray *arr = callee_val.as.array;
                    
                    // 3. Hacer el cast del puntero de datos genérico a (char *) 
                    // asegurando que data contenga la cadena de caracteres
                    char *func_name = (char *)arr->data;
                    
                    if (func_name) {
                        // 4. Buscar y ejecutar la función nativa registrada con ese nombre
                        FoxyNativeFunc native_fn = f_vm_find_native(vm, func_name);
                        if (native_fn) {
                            native_fn(vm, NULL, arg_count);
                        } else {
                            fprintf(stderr, "[Foxy Runtime Error] Función nativa no encontrada: %s\n", func_name);
                            vm->running = false;
                        }
                    }
                } else {
                    fprintf(stderr, "[Foxy Runtime Error] El identificador de llamada no es un arreglo de caracteres válido (tipo: %d).\n", callee_val.type);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_RET: {
                if (proc->stack_top == 0) {
                    f_vm_push(proc, FOXY_NULL_VALUE);
                }
                proc->state = FOXY_PROCESS_READY;
                return 0;
            }

            /* ==========================================
             * CONCURRENCIA Y PROCESOS
             * ========================================== */
            case FOXCODE_POPEN: {
            // 1. Extraer argumentos de la pila (en orden inverso)
            FoxyValue env_val   = f_vm_pop(proc); // Protocolo / SharedEnv (si aplica)
            FoxyValue name_val  = f_vm_pop(proc); // Nombre del proceso ("proceso 1")
            FoxyValue fn_val    = f_vm_pop(proc); // Función / Callback con el bytecode

            // 2. Extraer puntero al protocolo si se pasó un SharedEnv válido (vía FOXY_VAL_OBJECT)
            FoxyProtocol *protocol = NULL;
            if (env_val.type == FOXY_VAL_OBJECT) {
                protocol = (FoxyProtocol *)env_val.as.ptr;
            }

            // Las cadenas se validan como arreglos de caracteres (FOXY_VAL_ARRAY)
            const char *pname = NULL;
            if (name_val.type == FOXY_VAL_ARRAY) {
                pname = f_value_get_char_array_data(&name_val);
            } else {
                pname = "main_subproc"; // O un valor por defecto / manejo de error seguro
            }

            // Validación segura de la función y su bytecode
            if (!fn_val.as.function) {
                f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                    "POPEN requiere una función válida como callback de bytecode.");
                return FOXY_CAT_ERROR;
            }
            const uint8_t *sub_bytecode = fn_val.as.function->bytecode;

            // 3. Crear el subproceso e insertarlo en el runtime
            FoxyProcess *sub_proc = f_process_create(vm->runtime, pname, sub_bytecode, protocol);
            if (!sub_proc) {
                f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                    "No se pudo instanciar el proceso '%s'", pname);
                return FOXY_CAT_ERROR;
            }

            // 4. Iniciar ejecución asíncrona en hilo POSIX sin bloquear este proceso
            if (!f_process_start(sub_proc)) {
                f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME, 
                    "Fallo al iniciar el hilo del proceso '%s'", pname);
                return FOXY_CAT_ERROR;
            }

            // 5. Apilar la referencia del SUBPROCESO creado (sub_proc, no el padre proc)
            FoxyValue proc_obj;
            proc_obj.type = FOXY_VAL_OBJECT; 
            proc_obj.as.ptr = sub_proc; // <--- Apuntamos al subproceso recién creado
            f_vm_push(proc, proc_obj);  
            break;
        }

            case FOXCODE_ENV: {
                // Apila una referencia base para la construccion/resolucion de un SharedEnv
                FoxyValue env_val;
                env_val.type = FOXY_VAL_NULL;
                env_val.as.ptr = NULL;
                f_vm_push(proc, env_val);
                break;
            }

            case FOXCODE_ENV_CREATE: {
                // Lee o desapila el nombre que identificará al SharedEnv / Protocolo
                FoxyValue env_name_val = f_vm_pop(proc);
                
                const char *env_name = NULL;
                // Las cadenas se validan como arreglos de caracteres (FOXY_VAL_ARRAY)
                if (env_name_val.type == FOXY_VAL_ARRAY) {
                    env_name = f_value_get_char_array_data(&env_name_val);
                } else {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                        "ENV_CREATE requiere un arreglo de caracteres como nombre de protocolo.");
                    return FOXY_CAT_ERROR;
                }

                // Instancia o recupera el protocolo dentro del contenedor global (rt->protocols)
                FoxyProtocol *protocol = f_protocol_get_or_create(vm->runtime, env_name);
                if (!protocol) {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                        "Error al instanciar el SharedEnv '%s'.", env_name ? env_name : "");
                    return FOXY_CAT_ERROR;
                }

                // Apila el puntero env envuelto en un FoxyValue de tipo objeto
                FoxyValue prot_val;
                prot_val.type = FOXY_VAL_OBJECT; // <--- Cambiado de FOXY_VAL_PROTOCOL a FOXY_VAL_OBJECT
                prot_val.as.ptr = protocol;
                f_vm_push(proc, prot_val);
                break;
            }

            case FOXCODE_ENV_BIND: {
                // Recibe el objeto proceso y el objeto protocolo para asociarlos
                FoxyValue env_val  = f_vm_pop(proc);
                FoxyValue proc_val = f_vm_pop(proc);

                // Se validan como FOXY_VAL_OBJECT ya que no son tipos primitivos independientes
                if (env_val.type != FOXY_VAL_OBJECT || proc_val.type != FOXY_VAL_OBJECT) {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                        "ENV_BIND requiere un tipo Objeto (Proceso) y un tipo Objeto (Protocolo) validos.");
                    return FOXY_CAT_ERROR;
                }

                FoxyProcess *target_proc = (FoxyProcess *)proc_val.as.ptr;
                FoxyProtocol *protocol   = (FoxyProtocol *)env_val.as.ptr;

                if (target_proc && protocol) {
                    target_proc->protocol = protocol;
                } else {
                    f_utils_write_runtime_error(vm, FOXY_TOKEN_ERROR_RUNTIME,
                        "Punteros nulos al intentar realizar ENV_BIND.");
                    return FOXY_CAT_ERROR;
                }

                // Deja el proceso en la pila como valor de retorno de la operacion
                f_vm_push(proc, proc_val);
                break;
            }

            default:
                fprintf(stderr, "[Foxy VM Error] Unknown foxcode: 0x%02X at IP %zu\n", foxcode, proc->ip - 1);
                vm->running = false;
                break;
        }
    }

    return 0;
}

FoxyVM* f_vm_new(void) {
    FoxyVM *vm = (FoxyVM*) malloc(sizeof(FoxyVM));
    if (!vm) return NULL;

    // Inicialización del subsistema de constante y ejecución
    vm->constants = NULL;
    vm->constants_count = 0;
    vm->constants_capacity = 0;

    // Inicialización del subsistema de símbolos y extensiones C
    vm->native_symbols = NULL;
    vm->native_symbols_count = 0;
    vm->native_symbols_capacity = 0;

    // Inicialización del subsistema de librerías cargadas (openlib/modules)
    vm->loaded_libs = NULL;
    vm->loaded_libs_count = 0;
    vm->loaded_libs_capacity = 0;

    // Inicialización del subsistema de concurrencia y procesos
    vm->processes = NULL;
    vm->process_count = 0;
    vm->process_capacity = 0;

    return vm;
}

void f_vm_free(FoxyVM *vm) {
    if (!vm) return;

    // 1. Símbolos nativos
    if (vm->native_symbols) {
        free(vm->native_symbols);
        vm->native_symbols = NULL;
    }

    // 2. Librerías cargadas
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

    // 3. Tabla de constantes
    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            if (vm->constants[i].type == FOXY_VAL_OBJECT && vm->constants[i].as.obj) {
                // Si existe un destructor específico de objetos, úsalo aquí:
                // f_object_free(vm->constants[i].as.obj);
                free(vm->constants[i].as.obj);
            }
        }
        free(vm->constants);
        vm->constants = NULL;
    }
    vm->constants_count = 0;

    // 4. Procesos
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

    // 5. Instancia de la VM
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
