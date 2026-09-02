#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_vm.h"
#include "f_foxcode.h"
#include "f_parser.h"
#include "f_utils.h"
#include "f_codegen.h"
#include "f_openlib.h"
#include "f_dict.h"
#include "f_object.h"
#include "f_value.h"

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
            fprintf(stderr, "[Foxy VM Error] Memory allocation failed for process stack (PID %u)\n", p->pid);
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
    if (proc->bytecode) free(proc->bytecode);
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
            fprintf(stderr, "[Foxy VM Error] Out of memory expanding process table\n");
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
    memcpy(proc->bytecode, code, code_size);
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
        fprintf(stderr, "[Foxy VM Error] Stack Underflow (PID %d)\n", p ? (int)p->pid : -1);
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

int f_vm_run(FoxyVM *vm) {
    if (!vm || vm->process_count == 0) return 0;

    vm->running = true;
    FoxyProcess *proc = vm->processes[vm->current_process_index];
    proc->state = FOXY_PROCESS_RUNNING;

    while (vm->running && proc->ip < proc->bytecode_size) {
        uint8_t opcode = proc->bytecode[proc->ip++];

        switch (opcode) {
            case FOXCODE_NOP:
                // Avanza proc->ip implícitamente en la lectura y pasa al siguiente ciclo
                break;

            case FOXCODE_HALT:
                proc->running = 0;
                proc->state = FOXY_PROCESS_DEAD;
                vm->running = false;
                break;

            /* ==========================================
             * CONSTANTES Y LITERALES
             * ========================================== */
            case FOXCODE_LOAD_CONST: {
                uint8_t const_idx = proc->bytecode[proc->ip++];
                if (const_idx < vm->constants_count) {
                    f_vm_push(proc, vm->constants[const_idx]);
                } else {
                    fprintf(stderr, "[Foxy VM Error] Constant index out of bounds: %u\n", const_idx);
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
                uint8_t local_idx = proc->bytecode[proc->ip++];
                if (local_idx < proc->locals_count) {
                    f_vm_push(proc, proc->locals[local_idx]);
                } else {
                    fprintf(stderr, "[Foxy VM Error] Local variable index out of bounds: %u\n", local_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_STORE_LOCAL: {
                uint8_t local_idx = proc->bytecode[proc->ip++];
                FoxyValue val = f_vm_pop(proc);

                // Redimensionar el arreglo de variables locales en el proceso si es necesario
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
                uint8_t global_idx = proc->bytecode[proc->ip++];
                if (global_idx < vm->class_count && vm->classes) {
                    // Si tus globales están asociadas a vm->classes
                    f_vm_push(proc, FOXY_NULL_VALUE); 
                } else {
                    fprintf(stderr, "[Foxy VM Error] Global index out of bounds: %u\n", global_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_STORE_GLOBAL: {
                uint8_t global_idx = proc->bytecode[proc->ip++];
                FoxyValue val = f_vm_pop(proc);
                (void)val;
                (void)global_idx;
                break;
            }

            case FOXCODE_LOAD_LIB: {
                uint8_t lib_name_idx = proc->bytecode[proc->ip++];
                const char *lib_name = NULL;

                if (lib_name_idx < vm->constants_count) {
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
                    fprintf(stderr, "[Foxy VM Error] Constant index %u is not a valid CHAR array for FOXCODE_LOAD_LIB\n", lib_name_idx);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_SET_MEMBER: {
                // 1. Obtener el índice del nombre del miembro desde las constantes
                uint8_t member_idx = proc->bytecode[proc->ip++];
                const char *member_name = NULL;

                if (member_idx < vm->constants_count) {
                    FoxyValue c = vm->constants[member_idx];
                    if (c.type == FOXY_VAL_ARRAY && c.as.array && c.as.array->data) {
                        member_name = (const char *)c.as.array->data;
                    }
                }

                if (!member_name) {
                    fprintf(stderr, "[Foxy VM Error] Índice de constante %u inválido para SET_MEMBER\n", member_idx);
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }

                // 2. Desapilar el valor a asignar y el objeto contenedor
                FoxyValue val_to_assign = f_vm_pop(proc);
                FoxyValue target = f_vm_pop(proc);

                // 3. Asignar según el tipo de contenedor
                if (target.type == FOXY_VAL_DICT && target.as.dict) {
                    f_dict_set(target.as.dict, member_name, val_to_assign);

                } else if (target.type == FOXY_VAL_OBJECT && target.as.obj) {
                    // Uso directo de la API definida en f_object.h
                    f_object_set_field(target.as.obj, member_name, val_to_assign);

                } else {
                    fprintf(stderr, "[Foxy VM Error] Intento de escribir el miembro '%s' en un tipo no mutables (%s)\n",
                            member_name, f_value_type_to_string(target.type));
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }
                break;
            }

            case FOXCODE_GET_MEMBER: {
                uint8_t member_idx = proc->bytecode[proc->ip++];
                const char *member_name = NULL;

                if (member_idx < vm->constants_count) {
                    FoxyValue c = vm->constants[member_idx];
                    if (c.type == FOXY_VAL_ARRAY && c.as.array && c.as.array->data) {
                        member_name = (const char *)c.as.array->data;
                    }
                }

                if (!member_name) {
                    fprintf(stderr, "[Foxy VM Error] Índice de constante %u inválido para GET_MEMBER\n", member_idx);
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
                    break;
                }

                FoxyValue target = f_vm_pop(proc);
                FoxyValue result = { .type = FOXY_VAL_NULL };

                if (target.type == FOXY_VAL_DICT && target.as.dict) {
                    // Corrección: Pasar &result como tercer argumento
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
             * CONTROL DE FLUJO
             * ========================================== */
            case FOXCODE_JUMP: {
                uint16_t offset = (uint16_t)(proc->bytecode[proc->ip] << 8 | proc->bytecode[proc->ip + 1]);
                proc->ip += 2;
                proc->ip = offset;
                break;
            }

            case FOXCODE_JUMP_IF_FALSE: {
                uint16_t offset = (uint16_t)(proc->bytecode[proc->ip] << 8 | proc->bytecode[proc->ip + 1]);
                proc->ip += 2;
                FoxyValue condition = f_vm_pop(proc);
                if (!condition.as.boolean) {
                    proc->ip = offset;
                }
                break;
            }

            case FOXCODE_CALL: {
                uint8_t arg_count = proc->bytecode[proc->ip++];

                // Desapilar la función objetivo
                FoxyValue target_func = f_vm_pop(proc);

                if (target_func.type == FOXY_VAL_FUNCTION) {
                    if (target_func.as.native_fn != NULL) {
                        // 1. Invocación de Función Nativa C
                        typedef FoxyValue (*FoxyNativeVmFn)(FoxyVM *);
                        FoxyNativeVmFn vm_fn = (FoxyNativeVmFn)target_func.as.native_fn;
                        FoxyValue res = vm_fn(vm);
                        f_vm_push(proc, res);

                    } else if (target_func.as.func != NULL) {
                        // 2. Invocación de Función Interpretada Foxy
                        FoxyFunction *fn = target_func.as.func;

                        // Transferir los argumentos de la pila a las variables locales de la función
                        for (int i = (int)arg_count - 1; i >= 0; i--) {
                            proc->locals[i] = f_vm_pop(proc);
                        }

                        // Silenciar temporalmente la variable 'fn' si aún no se usan sus campos (ej. fn->chunk_offset)
                        (void)fn;

                        // Guardar el ip de retorno si la estructura maneja frames o saltar al offset
                        // proc->ip = fn->chunk_offset;

                    } else {
                        fprintf(stderr, "[Foxy VM Error] Puntero de función nulo\n");
                        proc->running = 0;
                        proc->state = FOXY_PROCESS_DEAD;
                    }
                } else {
                    fprintf(stderr, "[Foxy VM Error] El tipo %s no es una función ejecutable\n",
                            f_value_type_to_string(target_func.type));
                    proc->running = 0;
                    proc->state = FOXY_PROCESS_DEAD;
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
            case FOXCODE_POPEN:
            case FOXCODE_ENV_CREATE:
            case FOXCODE_ENV_BIND:
                break;

            default:
                fprintf(stderr, "[Foxy VM Error] Unknown opcode: 0x%02X at IP %zu\n", opcode, proc->ip - 1);
                vm->running = false;
                break;
        }
    }

    return 0;
}

void f_vm_free(FoxyVM *vm) {
    if (!vm) return;

    if (vm->native_symbols)
        free(vm->native_symbols);

    if (vm->loaded_libs) {
        for (size_t i = 0; i < vm->loaded_libs_count; i++)
            free(vm->loaded_libs[i]);
        free(vm->loaded_libs);
    }

    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            if (vm->constants[i].type == FOXY_VAL_OBJECT && vm->constants[i].as.obj)
                free(vm->constants[i].as.obj);
        }
        free(vm->constants);
    }

    if (vm->processes) {
        for (size_t i = 0; i < vm->process_count; i++) {
            f_process_free(vm->processes[i]);
        }
        free(vm->processes);
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
