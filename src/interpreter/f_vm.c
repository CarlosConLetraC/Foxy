#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_vm.h"
#include "f_parser.h"
#include "f_utils.h"
#include "f_codegen.h"

FoxyVM* f_vm_create(void) {
    FoxyVM* vm = (FoxyVM*)malloc(sizeof(FoxyVM));
    if (!vm) return NULL;

    vm->process_capacity = 8;
    vm->process_count = 0;
    vm->processes = (FoxyProcess**)malloc(sizeof(FoxyProcess*) * vm->process_capacity);
    vm->current_process_index = 0;

    // Inicializar el arreglo de librerías cargadas (loadedlibs)
    vm->loaded_libs_capacity = 4;
    vm->loaded_libs_count = 0;
    vm->loaded_libs = (char**)malloc(sizeof(char*) * vm->loaded_libs_capacity);

    vm->running = false;
    return vm;
}

bool f_vm_load_process(FoxyVM* vm, const uint8_t* code, size_t size, const char* name) {
    if (vm->process_count >= vm->process_capacity) {
        vm->process_capacity *= 2;
        FoxyProcess** temp = realloc(vm->processes, sizeof(FoxyProcess*) * vm->process_capacity);
        if (!temp) return false;
        vm->processes = temp;
    }

    FoxyProcess* proc = (FoxyProcess*)malloc(sizeof(FoxyProcess));
    if (!proc) return false;

    proc->pid = (int)vm->process_count + 1;
    snprintf(proc->name, sizeof(proc->name), "%s", name ? name : "main_process");
    proc->state = FOXY_PROCESS_READY;
    proc->bytecode = code;
    proc->bytecode_size = size;
    proc->ip = 0;

    // Asignar una pila virtual básica (256 elementos de 64 bits)
    proc->stack = (uint64_t*)malloc(sizeof(uint64_t) * 256);
    proc->stack_top = 0;

    vm->processes[vm->process_count++] = proc;
    return true;
}

bool f_vm_load_script_to_process(FoxyVM* vm, ASTNode* ast_root, int pid) {
    // 1. Generar el blob binario empaquetado (Constant Pool + Bytecode) desde el AST
    size_t blob_size = 0;
    uint8_t* blob = f_generate_bytecode(ast_root, &blob_size);
    if (!blob) return false;

    size_t offset = 0;

    // 2. Leer el número de constantes globales (uint32_t)
    if (offset + sizeof(uint32_t) > blob_size) { free(blob); return false; }
    uint32_t num_consts;
    memcpy(&num_consts, blob + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Asignar el Constant Pool en la VM global
    vm->constants_count = num_consts;
    vm->constants = malloc(sizeof(FoxyConstant) * num_consts);

    // 3. Leer cada constante del blob y guardarlas en la VM
    for (uint32_t i = 0; i < num_consts; i++) {
        if (offset + 1 + sizeof(uint16_t) > blob_size) { free(blob); return false; }
        
        uint8_t ctype = blob[offset++];
        vm->constants[i].type = (FOXY_CONSTANT_TYPE)ctype;

        uint16_t slen;
        memcpy(&slen, blob + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if (offset + slen > blob_size) { free(blob); return false; }

        char* str_val = malloc(slen + 1);
        memcpy(str_val, blob + offset, slen);
        str_val[slen] = '\0';
        vm->constants[i].as.sval = str_val;
        offset += slen;
    }

    // 4. Leer el tamaño del bytecode de instrucciones (uint32_t)
    if (offset + sizeof(uint32_t) > blob_size) { free(blob); return false; }
    uint32_t code_size;
    memcpy(&code_size, blob + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 5. Extraer únicamente el flujo de bytecode puro
    if (offset + code_size > blob_size) { free(blob); return false; }
    uint8_t* pure_bytecode = malloc(code_size);
    memcpy(pure_bytecode, blob + offset, code_size);

    // 6. Usar tu función nativa f_vm_load_process para registrar el proceso formalmente
    char proc_name[32];
    snprintf(proc_name, sizeof(proc_name), "process_%d", pid);
    bool success = f_vm_load_process(vm, pure_bytecode, code_size, proc_name);

    // Asignar el PID correcto al último proceso creado si es necesario
    if (success && vm->process_count > 0) {
        vm->processes[vm->process_count - 1]->pid = pid;
    }

    // Limpiar el blob temporal en bruto
    free(blob);
    return success;
}

int f_vm_run(FoxyVM* vm) {
    if (vm->process_count == 0) return 0;
    vm->running = true;

    // Ciclo del Task Manager (Round-Robin básico)
    while (vm->running) {
        bool active_found = false;

        for (size_t i = 0; i < vm->process_count; i++) {
            FoxyProcess* p = vm->processes[i];

            if (p->state == FOXY_PROCESS_DEAD) continue;
            active_found = true;
            p->state = FOXY_PROCESS_RUNNING;

            // Ejecutar un "quantum" de instrucciones o hasta que termine el bytecode
            int opcodes_executed = 0;
            while (opcodes_executed < 100 && p->ip < p->bytecode_size) {
                uint8_t opcode = p->bytecode[p->ip++];

                switch (opcode) {
                    case FOXCODE_LOAD_LIB: {
                        // 1. Leer el índice de la constante desde el bytecode de instrucciones
                        if (p->ip < p->bytecode_size) {
                            uint8_t const_index = p->bytecode[p->ip++];
                            
                            // 2. Obtener la ruta desde el Constant Pool de la VM
                            if (const_index < vm->constants_count && vm->constants[const_index].type == FOXY_CONSTANT_CHARARRAY) {
                                const char* lib_path = vm->constants[const_index].as.sval;

                                // 3. Verificar si la librería ya está cargada en loadedlibs
                                bool already_loaded = false;
                                for (size_t l = 0; l < vm->loaded_libs_count; l++) {
                                    if (strcmp(vm->loaded_libs[l], lib_path) == 0) {
                                        already_loaded = true;
                                        break;
                                    }
                                }

                                // 4. Si no está cargada, registrarla y cargarla
                                if (!already_loaded) {
                                    if (vm->loaded_libs_count >= vm->loaded_libs_capacity) {
                                        vm->loaded_libs_capacity = vm->loaded_libs_capacity ? vm->loaded_libs_capacity * 2 : 4;
                                        char** temp = realloc(vm->loaded_libs, sizeof(char*) * vm->loaded_libs_capacity);
                                        if (temp) vm->loaded_libs = temp;
                                    }
                                    vm->loaded_libs[vm->loaded_libs_count++] = strdup(lib_path);
                                    f_utils_load_native_lib(vm, lib_path);
                                    printf("[Foxy VM] Librería cargada y registrada: %s\n", lib_path);
                                } else {
                                    printf("[Foxy VM] Librería ya existente en loadedlibs: %s\n", lib_path);
                                }
                            } else {
                                fprintf(stderr, "[Foxy VM Error] Índice de constante inválido o no es un chararray para LOAD_LIB\n");
                            }
                        }
                        break;
                    }

                    case FOXCODE_NOP:
                        // No hace nada
                        break;

                    case FOXCODE_HALT:
                        p->state = FOXY_PROCESS_DEAD;
                        vm->running = false;
                        break;

                    case FOXCODE_LOAD_NULL:
                        p->stack[p->stack_top++] = 0; // Representación de NULL
                        break;

                    case FOXCODE_LOAD_TRUE:
                        p->stack[p->stack_top++] = 1;
                        break;

                    case FOXCODE_LOAD_FALSE:
                        p->stack[p->stack_top++] = 0;
                        break;
                    
                    case FOXCODE_LOAD_CONST: {
                        if (p->ip < p->bytecode_size) {
                            uint8_t const_index = p->bytecode[p->ip++];
                            
                            if (const_index < vm->constants_count) {
                                FoxyConstant* c = &vm->constants[const_index];
                                
                                // Opcional: imprimir para depuración si es texto
                                if (c->type == FOXY_CONSTANT_CHARARRAY) {
                                    printf("[Foxy Runtime String] %s\n", c->as.sval);
                                }

                                // ¡CRUCIAL!: Empujar el índice de la constante (o su valor) al stack 
                                // para que la siguiente instrucción (STORE_LOCAL, CALL, etc.) lo consuma.
                                if (p->stack_top < 256) { // Asegurar que no desbordes la pila básica de 256
                                    p->stack[p->stack_top++] = (uint64_t)const_index;
                                } else {
                                    fprintf(stderr, "[Foxy VM Error] Stack overflow en LOAD_CONST\n");
                                    p->state = FOXY_PROCESS_DEAD;
                                    return -1;
                                }

                            } else {
                                fprintf(stderr, "[Foxy VM Error] Índice de constante fuera de rango en LOAD_CONST: %u (Total: %u)\n", (uint32_t)const_index, vm->constants_count);
                                p->state = FOXY_PROCESS_DEAD;
                                return -1;
                            }
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Fin de bytecode inesperado en LOAD_CONST\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }
                    case FOXCODE_STORE_LOCAL: {
                        // Verificar que hay al menos 1 byte disponible para leer el índice del local
                        if (p->ip < p->bytecode_size) {
                            // 1. Consumir el operando de 1 byte que indica el índice de la variable local
                            uint8_t local_index = p->bytecode[p->ip++];
                            
                            // 2. Verificar que la pila tenga un valor disponible para almacenar
                            if (p->stack_top > 0) {
                                uint64_t val = p->stack[--p->stack_top];
                                
                                // 3. Guardar el valor en el almacenamiento local del proceso.
                                // (Ajusta esta línea según cómo manejes los locales en tu estructura FoxyProcess, 
                                // por ejemplo: p->locals[local_index] = val;)
                                (void)val;         // Placeholder para evitar warnings si aún no tienes el arreglo de locales
                                (void)local_index; // Placeholder para evitar warnings
                                
                            } else {
                                fprintf(stderr, "[Foxy VM Error] Pila vacía al intentar ejecutar STORE_LOCAL (PID %d)\n", p->pid);
                                p->state = FOXY_PROCESS_DEAD;
                                return -1;
                            }
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Fin de bytecode inesperado en STORE_LOCAL (PID %d)\n", p->pid);
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_CALL: {
                        // Como CALL (0x15) es un opcode de un solo byte sin operandos inline,
                        // no consumimos ningún byte extra de p->ip.

                        // Verificamos que la pila tenga elementos para ejecutar la llamada
                        if (p->stack_top > 0) {
                            // Extraer la referencia o el valor superior de la pila (ej. el índice de la constante o argumento)
                            uint64_t call_target = p->stack[--p->stack_top];
                            
                            // Si el target en la pila corresponde a una constante o función cargada, 
                            // aquí puedes invocar la acción correspondiente.
                            (void)call_target;

                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila vacía o desbalanceada al intentar ejecutar CALL (PID %d)\n", p->pid);
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }
                    
                    case FOXCODE_EQ: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            p->stack[p->stack_top++] = (a == b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para EQ\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_NEQ: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            p->stack[p->stack_top++] = (a != b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para NEQ\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_LT: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            // Usamos casteo a int64_t si manejas números con signo
                            p->stack[p->stack_top++] = ((int64_t)a < (int64_t)b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para LT\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_GT: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            p->stack[p->stack_top++] = ((int64_t)a > (int64_t)b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para GT\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_LE: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            p->stack[p->stack_top++] = ((int64_t)a <= (int64_t)b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para LE\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    case FOXCODE_GE: {
                        if (p->stack_top >= 2) {
                            uint64_t b = p->stack[--p->stack_top];
                            uint64_t a = p->stack[--p->stack_top];
                            p->stack[p->stack_top++] = ((int64_t)a >= (int64_t)b) ? 1 : 0;
                        } else {
                            fprintf(stderr, "[Foxy VM Error] Pila insuficiente para GE\n");
                            p->state = FOXY_PROCESS_DEAD;
                            return -1;
                        }
                        break;
                    }

                    default:
                        fprintf(stderr, "[Foxy VM Error] Opcode desconocido: 0x%02X en PID %d\n", opcode, p->pid);
                        p->state = FOXY_PROCESS_DEAD;
                        return -1;
                }

                opcodes_executed++;
            }

            if (p->ip >= p->bytecode_size)
                p->state = FOXY_PROCESS_DEAD;
        }

        if (!active_found)
            vm->running = false; // Todos los procesos terminaron
    }

    return 0;
}

void f_process_free(FoxyProcess* proc) {
    if (!proc) return;

    // Liberar el bytecode puro asignado al proceso
    if (proc->bytecode) {
        free((void*)proc->bytecode); // Remueve el const si tu estructura usa uint8_t* puro
    }

    // Liberar la pila virtual
    if (proc->stack) {
        free(proc->stack);
    }
    
    free(proc);
}

void f_vm_free(FoxyVM* vm) {
    if (!vm) return;
    
    // 1. Liberar procesos activos/muertos
    if (vm->processes) {
        for (size_t i = 0; i < vm->process_count; i++) {
            f_process_free(vm->processes[i]);
        }
        free(vm->processes);
    }

    // 2. Liberar librerías dinámicas cargadas
    if (vm->loaded_libs) {
        for (size_t i = 0; i < vm->loaded_libs_count; i++) {
            free(vm->loaded_libs[i]);
        }
        free(vm->loaded_libs);
    }

    // 3. Liberar el Constant Pool global de la VM y sus cadenas internas
    if (vm->constants) {
        for (size_t i = 0; i < vm->constants_count; i++) {
            if (vm->constants[i].type == FOXY_CONSTANT_CHARARRAY && vm->constants[i].as.sval) {
                free(vm->constants[i].as.sval);
                vm->constants[i].as.sval = NULL; // Previene doble liberación
            }
        }
        free(vm->constants);
        vm->constants = NULL;
    }

    free(vm);
}