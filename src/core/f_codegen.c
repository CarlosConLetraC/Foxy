#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_codegen.h"
#include "f_foxcode.h"
#include "f_value.h"
#include "f_ast.h"

void f_codegen_init(FoxyCodegen *cg) {
    if (!cg) return;
    
    cg->code_count = 0;
    cg->code_capacity = 256;
    cg->bytecode = malloc(sizeof(FoxInstruction) * cg->code_capacity);

    cg->constants_count = 0;
    cg->constants_capacity = 64;
    cg->constants = malloc(sizeof(FoxyValue) * cg->constants_capacity);
}

void f_codegen_free(FoxyCodegen *cg) {
    if (!cg) return;
    
    free(cg->bytecode);
    free(cg->constants);
    
    cg->bytecode = NULL;
    cg->constants = NULL;
    cg->code_count = 0;
    cg->constants_count = 0;
    cg->code_capacity = 0;
    cg->constants_capacity = 0;
}

size_t f_codegen_emit(FoxyCodegen *cg, FoxInstruction inst) {
    if (!cg) return 0;

    if (cg->code_count >= cg->code_capacity) {
        cg->code_capacity *= 2;
        FoxInstruction *new_bytecode = realloc(cg->bytecode, sizeof(FoxInstruction) * cg->code_capacity);
        if (!new_bytecode) {
            fprintf(stderr, "[Foxy Codegen Error] Out of memory reallocating bytecode buffer\n");
            exit(1);
        }
        cg->bytecode = new_bytecode;
    }
    
    cg->bytecode[cg->code_count] = inst;
    return cg->code_count++;
}

void f_codegen_emit_byte(FoxyCodegen *cg, uint8_t opcode) {
    if (!cg) return;
    f_codegen_emit(cg, CREATE_ABC((FoxOpcode)opcode, 0, 0, 0));
}

int f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val) {
    if (!cg) return -1;

    if (cg->constants_count >= cg->constants_capacity) {
        cg->constants_capacity *= 2;
        FoxyValue *new_constants = realloc(cg->constants, sizeof(FoxyValue) * cg->constants_capacity);
        if (!new_constants) {
            fprintf(stderr, "[Foxy Codegen Error] Out of memory reallocating constant pool\n");
            exit(1);
        }
        cg->constants = new_constants;
    }
    
    cg->constants[cg->constants_count] = val;
    return (int)(cg->constants_count++);
}

FoxyASTNode* f_ast_create_var_decl(const char *name, FoxyASTNode *initializer) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_VAR_DECL);
    if (!node) return NULL;
    node->as.var_decl_node.name = strdup(name);
    node->as.var_decl_node.initializer = initializer;
    return node;
}

static int f_codegen_resolve_local(FoxyCodegen *cg, const char *name) {
    // 1. Buscar si la variable ya existe en el ámbito local
    for (int i = (int)cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) {
            return i; // Retorna el índice existente
        }
    }

    // 2. Si no existe, registrarla automáticamente como nueva variable local
    if (cg->local_count < FOXY_MAX_LOCALS) {
        int new_idx = (int)cg->local_count++;
        
        // Copiar el nombre de forma segura al arreglo estático
        strncpy(cg->locals[new_idx].name, name, sizeof(cg->locals[new_idx].name) - 1);
        cg->locals[new_idx].name[sizeof(cg->locals[new_idx].name) - 1] = '\0';
        
        return new_idx;
    }

    fprintf(stderr, "[Foxy Codegen Error] Límite de variables locales excedido (%s)\n", name);
    return 0;
}

/**
 * Función principal del patrón Visitor para recorrer y compilar nodos del AST.
 */
bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!node || !cg) return false;

    switch (node->type) {
        case FOXY_AST_NODE_PROGRAM: {
            for (size_t i = 0; i < node->as.program_node.count; i++) {
                if (!f_codegen_visit(cg, node->as.program_node.statements[i])) {
                    return false;
                }
            }
            break;
        }

        case FOXY_AST_NODE_INCLUDE: {
            const char *path_str = node->as.include_node.path;
            FoxyValue path_val = f_value_create_char_array(path_str, strlen(path_str));

            int const_idx = f_codegen_add_constant(cg, path_val);
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_INCLUDE, 0, (uint16_t)const_idx));
            break;
        }

        case FOXY_AST_NODE_EXPR_STMT: {
            if (node->as.expr_stmt_node.expression) {
                if (!f_codegen_visit(cg, node->as.expr_stmt_node.expression)) {
                    return false;
                }
                // Si la expresión deja un valor temporal en la pila que no se usará,
                // puedes emitir una instrucción pop si la tienes disponible:
                // f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0));
            }
            break;
        }

        case FOXY_AST_NODE_LITERAL: {
            int const_idx = f_codegen_add_constant(cg, node->as.literal_node.value);
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
            break;
        }

        case FOXY_AST_NODE_CALL: {
            // 1. Evaluar argumentos
            for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
                if (!f_codegen_visit(cg, node->as.call_node.arguments[i])) {
                    return false;
                }
            }

            // 2. Cargar el callee
            const char *callee = node->as.call_node.callee_name;
            FoxyValue func_val = f_value_create_char_array(callee, strlen(callee));

            int func_const_idx = f_codegen_add_constant(cg, func_val);
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)func_const_idx));

            int arg_count = (int)node->as.call_node.arg_count;
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_CALL, (uint8_t)arg_count, 0, 0));
            break;
        }

        case FOXY_AST_NODE_IDENTIFIER: {
            const char *id_name = node->as.identifier_node.name;
            int local_idx = f_codegen_resolve_local(cg, id_name);
            
            // Emitir carga de variable local en lugar de global
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_LOAD_LOCAL, (uint8_t)local_idx, 0, 0));
            break;
        }

        case FOXY_AST_NODE_ASSIGN: {
            // El nodo assign tiene left (identificador) y right (expresión)
            // 1. Compilar la expresión de la derecha (deja el valor en el tope de la pila)
            if (!f_codegen_visit(cg, node->as.binary_node.right)) return false;

            // 2. Obtener el nombre de la variable del nodo izquierdo (que es un IDENTIFIER)
            FoxyASTNode *left_node = node->as.binary_node.left;
            if (left_node->type != FOXY_AST_NODE_IDENTIFIER) {
                fprintf(stderr, "[Foxy Codegen Error] Destino de asignación inválido\n");
                return false;
            }

            const char *var_name = left_node->as.identifier_node.name;
            int local_idx = f_codegen_resolve_local(cg, var_name);

            // 3. Emitir la instrucción para guardar en la variable local
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0, 0));
            break;
        }

        case FOXY_AST_NODE_BINARY_OP: {
            if (!f_codegen_visit(cg, node->as.binary_node.left)) return false;
            if (!f_codegen_visit(cg, node->as.binary_node.right)) return false;

            FoxOpcode op_inst = FOXCODE_ADD;
            switch (node->as.binary_node.op_token) {
                case '+': op_inst = FOXCODE_ADD; break;
                case '-': op_inst = FOXCODE_SUB; break;
                case '*': op_inst = FOXCODE_MUL; break;
                case '/': op_inst = FOXCODE_DIV; break;
                default:  op_inst = FOXCODE_ADD; break;
            }
            f_codegen_emit(cg, CREATE_ABC(op_inst, 0, 0, 0));
            break;
        }

        case FOXY_AST_NODE_VAR_DECL: {
            // 1. Evaluar la expresión inicial (ej. el '0'), si existe
            if (node->as.var_decl_node.initializer) {
                if (!f_codegen_visit(cg, node->as.var_decl_node.initializer)) return false;
            }

            // 2. Resolver o registrar automáticamente la variable local en la tabla
            const char *var_name = node->as.var_decl_node.name;
            int local_idx = f_codegen_resolve_local(cg, var_name);

            // 3. Guardar el valor del tope de la pila en la variable local
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0, 0));
            break;
        }

        case FOXY_AST_NODE_FOR: {
            // 1. Inicialización opcional
            if (node->as.for_node.init) {
                if (!f_codegen_visit(cg, node->as.for_node.init)) return false;
            }

            size_t loop_start = cg->code_count;
            size_t exit_jump = (size_t)-1;

            // 2. Condición opcional
            if (node->as.for_node.condition) {
                if (!f_codegen_visit(cg, node->as.for_node.condition)) return false;
                // Salta si es falso (y la VM debe hacer pop del valor evaluado)
                exit_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));
            }

            // 3. Cuerpo del bucle
            if (node->as.for_node.body) {
                if (!f_codegen_visit(cg, node->as.for_node.body)) return false;
            }

            // 4. Incremento opcional
            if (node->as.for_node.increment) {
                if (!f_codegen_visit(cg, node->as.for_node.increment)) return false;
                
                // ¡IMPORTANTE! Limpiar el resultado que deja la expresión de incremento en la pila
                // (Si tienes una instrucción como FOXCODE_POP, emítela aquí):
                // f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0)); // o equivalente
            }

            // Salto de retorno al inicio (revalúa la condición)
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, (uint16_t)loop_start));

            // Parchar salida si hubo condición
            if (exit_jump != (size_t)-1) {
                size_t loop_end = cg->code_count;
                cg->bytecode[exit_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)loop_end);
            }
            break;
        }

        case FOXY_AST_NODE_ENV_CREATE: {
            f_codegen_visit_env_create(cg, node);
            break;
        }

        case FOXY_AST_NODE_ENV_BIND: {
            f_codegen_visit_env_bind(cg, node);
            break;
        }

        case FOXY_AST_NODE_POPEN: {
            f_codegen_visit_popen(cg, node);
            break;
        }

        default:
            fprintf(stderr, "[Foxy Codegen Warning] Nodo AST no soportado: %s\n", 
                f_ast_node_type_to_string(node->type));
            break;
    }

    return true;
}

/**
 * Emite bytecode para inicializar la semilla de un entorno.
 */
void f_codegen_emit_env(FoxyCodegen *cg) {
    if (!cg) return;
    f_codegen_emit_byte(cg, FOXCODE_ENV);
}

/**
 * Genera el código para crear un SharedEnv (Protocolo) registrado en el runtime.
 */
void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.env_create_node.name_expr != NULL) {
        f_codegen_visit(cg, node->as.env_create_node.name_expr);
    }

    f_codegen_emit_byte(cg, FOXCODE_ENV_CREATE);
}

/**
 * Genera el código para vincular un SharedEnv (Protocol) a un objeto Proceso.
 */
void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    // 1. Apilar proceso y protocolo
    f_codegen_visit(cg, node->as.env_bind_node.process_expr);
    f_codegen_visit(cg, node->as.env_bind_node.env_expr);

    // 2. Emitir instrucción de binding
    f_codegen_emit_byte(cg, FOXCODE_ENV_BIND);
}

/**
 * Genera la secuencia de bytecode para la llamada 'popen(...)'.
 */
void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    // 1. Callback de subproceso
    f_codegen_visit(cg, node->as.popen_node.callback_expr);

    // 2. Nombre del proceso
    if (node->as.popen_node.name_expr != NULL) {
        f_codegen_visit(cg, node->as.popen_node.name_expr);
    } else {
        f_codegen_emit_byte(cg, FOXCODE_LOAD_NULL);
    }

    // 3. Referencia al SharedEnv / Protocolo
    if (node->as.popen_node.env_expr != NULL) {
        f_codegen_visit(cg, node->as.popen_node.env_expr);
    } else {
        f_codegen_emit_byte(cg, FOXCODE_LOAD_NULL);
    }

    // 4. Emitir el opcode de instanciación
    f_codegen_emit_byte(cg, FOXCODE_POPEN);
}

bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root) {
    if (!cg || !ast_root) return false;

    if (!f_codegen_visit(cg, ast_root)) {
        return false;
    }

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));
    return true;
}