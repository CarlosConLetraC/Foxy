#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_vm.h"
#include "f_foxcode.h"
#include "f_parser.h"
#include "f_utils.h"
#include "f_codegen.h"
#include "f_openlib.h"

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

void f_vm_load_process(FoxyVM *vm, const uint8_t *code, size_t code_size, const char *filename) {
    if (!vm || !code || code_size == 0) return;

    if (vm->process_count >= vm->process_capacity) {
        size_t new_cap = vm->process_capacity * 2;
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
    memcpy(proc->bytecode, code, code_size);
    proc->bytecode_size = code_size;
    proc->ip = 0;

    proc->stack_capacity = 256;
    proc->stack = (FoxyValue *)malloc(sizeof(FoxyValue) * proc->stack_capacity);
    proc->stack_top = 0;

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
                break;

            case FOXCODE_HALT:
                proc->state = FOXY_PROCESS_DEAD;
                vm->running = false;
                break;

            case FOXCODE_LOAD_LIB: {
                uint8_t lib_name_idx = proc->bytecode[proc->ip++];
                const char *lib_name = NULL;

                if (lib_name_idx < vm->constants_count) {
                    FoxyConstant c = vm->constants[lib_name_idx];
                    lib_name = f_utils_get_string_from_constant(c);
                    fprintf(stderr, "[Foxy VM Debug] Direccion base de constants: %p\n", (void*)vm->constants);
                    fprintf(stderr, "[Foxy VM Debug] Direccion de vm->constants[0]: %p\n", (void*)&vm->constants[lib_name_idx]);
                    fprintf(stderr, "[Foxy VM Debug] Tipo de constante o campos crudos...\n");
                    FoxyConstant c0 = vm->constants[0];
                    fprintf(stderr, "[Foxy VM Debug] Constante 0 - Tipo: %d, Puntero/Valor crudo: %p\n", c0.type, (void*)c0.as.obj); // Ajusta 'as.obj' según el campo de tu unión
                }

                if (lib_name) {
                    fprintf(stderr, "[Foxy VM Debug] Leyendo constante en índice: %d\n", lib_name_idx);
                    if (!f_vm_load_library(vm, lib_name)) {
                        fprintf(stderr, "[Foxy VM Error] Could not load library: %s\n", lib_name);
                        vm->running = false;
                    }
                } else {
                    fprintf(stderr, "[Foxy VM Error] Invalid constant index for FOXCODE_LOAD_LIB at IP %zu\n", proc->ip - 2);
                    vm->running = false;
                }
                break;
            }

            case FOXCODE_LOAD_CONST: {
                uint8_t const_idx = proc->bytecode[proc->ip++];
                if (const_idx < vm->constants_count) {
                    f_vm_push(proc, vm->constants[const_idx]);
                }
                break;
            }

            case FOXCODE_LOAD_NULL: {
                f_vm_push(proc, FOXY_NULL_VALUE);
                break;
            }

            case FOXCODE_LOAD_TRUE: {
                FoxyValue val = { .type = FOXY_VAL_BOOL, .as.bval = true };
                f_vm_push(proc, val);
                break;
            }

            case FOXCODE_LOAD_FALSE: {
                FoxyValue val = { .type = FOXY_VAL_BOOL, .as.bval = false };
                f_vm_push(proc, val);
                break;
            }

            case FOXCODE_CALL: {
                uint8_t call_target = proc->bytecode[proc->ip++];
                uint8_t argc = proc->bytecode[proc->ip++];

                const char *func_name = NULL;
                if (call_target < vm->constants_count && vm->constants[call_target].type == FOXY_VAL_OBJECT) {
                    func_name = (const char *)vm->constants[call_target].as.obj;
                }

                if (func_name) {
                    FoxyNativeMethod native_func = f_vm_find_native(vm, func_name);
                    if (native_func) {
                        FoxyValue *args = &proc->stack[proc->stack_top - argc];
                        FoxyValue result = native_func(argc, args);
                        proc->stack_top -= argc;
                        f_vm_push(proc, result);
                    } else {
                        fprintf(stderr, "[Foxy VM Error] Native symbol not found: %s\n", func_name);
                        vm->running = false;
                    }
                }
                break;
            }

            default:
                fprintf(stderr, "[Foxy VM Error] Unknown opcode: 0x%02X at IP %zu\n", opcode, proc->ip - 1);
                vm->running = false;
                break;
        }
    }

    return 0;
}

void f_process_free(FoxyProcess* proc) {
    if (!proc) return;
    if (proc->bytecode) free((void*)proc->bytecode);
    if (proc->stack) free(proc->stack);
    free(proc);
}

void f_vm_free(FoxyVM *vm) {
    if (!vm) return;

    if (vm->native_symbols) {
        free(vm->native_symbols);
    }

    if (vm->loaded_libs) {
        for (size_t i = 0; i < vm->loaded_libs_count; i++) {
            free(vm->loaded_libs[i]);
        }
        free(vm->loaded_libs);
    }

    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            if (vm->constants[i].type == FOXY_VAL_OBJECT && vm->constants[i].as.obj) {
                free(vm->constants[i].as.obj);
            }
        }
        free(vm->constants);
    }

    if (vm->processes) {
        for (size_t i = 0; i < vm->process_count; i++) {
            FoxyProcess *p = vm->processes[i];
            if (p) {
                if (p->bytecode) free(p->bytecode);
                if (p->stack) free(p->stack);
                free(p);
            }
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
