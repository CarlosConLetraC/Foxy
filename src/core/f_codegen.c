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

    cg->local_count = 0;
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
    cg->local_count = 0;
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

static int f_codegen_resolve_local(FoxyCodegen *cg, const char *name) {
    for (int i = (int)cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) {
            return i;
        }
    }

    if (cg->local_count < FOXY_MAX_LOCALS) {
        int new_idx = (int)cg->local_count++;
        
        strncpy(cg->locals[new_idx].name, name, sizeof(cg->locals[new_idx].name) - 1);
        cg->locals[new_idx].name[sizeof(cg->locals[new_idx].name) - 1] = '\0';
        
        return new_idx;
    }

    fprintf(stderr, "[Foxy Codegen Error] Límite de variables locales excedido (%s)\n", name);
    return 0;
}

bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!node || !cg) return false;

    // 1. Reutilización directa de FOXY_AST_NODE_LIST de f_ast.h para la tabla de despacho
    #define MAKE_LABEL_PTR(enum_val, name_str) [enum_val] = &&lbl_##enum_val,
    static const void *dispatch_table[] = {
        FOXY_AST_NODE_LIST(MAKE_LABEL_PTR)
    };
    #undef MAKE_LABEL_PTR

    // 2. Validación de límites
    if ((size_t)node->type >= sizeof(dispatch_table) / sizeof(dispatch_table[0]) ||
        !dispatch_table[node->type]) {
        goto lbl_FOXY_AST_NODE_DEFAULT;
    }

    // 3. Salto indirecto
    goto *dispatch_table[node->type];

    lbl_FOXY_AST_NODE_PROGRAM: {
        for (size_t i = 0; i < node->as.program_node.count; i++) {
            if (!f_codegen_visit(cg, node->as.program_node.statements[i])) {
                return false;
            }
        }
        return true;
    }

    lbl_FOXY_AST_NODE_INCLUDE: {
        const char *path_str = node->as.include_node.path;
        FoxyValue path_val = f_value_create_char_array(path_str, strlen(path_str));

        int const_idx = f_codegen_add_constant(cg, path_val);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_INCLUDE, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_EXPR_STMT: {
        if (node->as.expr_stmt_node.expression) {
            if (!f_codegen_visit(cg, node->as.expr_stmt_node.expression)) {
                return false;
            }
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0));
        }
        return true;
    }

    lbl_FOXY_AST_NODE_CALL: {
        for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
            if (!f_codegen_visit(cg, node->as.call_node.arguments[i])) {
                return false;
            }
        }

        const char *callee = node->as.call_node.callee_name;
        FoxyValue func_val = f_value_create_char_array(callee, strlen(callee));

        int func_const_idx = f_codegen_add_constant(cg, func_val);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)func_const_idx));

        int arg_count = (int)node->as.call_node.arg_count;
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_CALL, (uint8_t)arg_count, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_LITERAL: {
        int const_idx = f_codegen_add_constant(cg, node->as.literal_node.value);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_IDENTIFIER: {
        const char *id_name = node->as.identifier_node.name;
        int local_idx = f_codegen_resolve_local(cg, id_name);
        
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_LOAD_LOCAL, (uint8_t)local_idx, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_BINARY_OP: {
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
        return true;
    }

    lbl_FOXY_AST_NODE_VAR_DECL: {
        // 1. PRIMERO: Evaluar el inicializador para empujar el FoxyValue al Top of Stack (TOS)
        /* printf("[CODEGEN DEBUG] Procesando var_decl: %s, initializer: %p\n",
           node->as.var_decl_node.name,
           (void*)node->as.var_decl_node.initializer); */
        if (node->as.var_decl_node.initializer) {
            /* printf("[CODEGEN DEBUG] Tipo de nodo initializer: %d\n",
               node->as.var_decl_node.initializer->type); */
            if (!f_codegen_visit(cg, node->as.var_decl_node.initializer)) {
                return false;
            }
        } else {
            // Si no se proporcionó inicializador (ej. 'int x'), empujamos FOXY_VAL_NULL
            FoxyValue null_val = { .type = FOXY_VAL_NULL, .as.ptr = NULL };
            int const_idx = f_codegen_add_constant(cg, null_val);
            if (const_idx < 0) return false;
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
        }

        // 2. Registrar o buscar la variable local en FoxyCodegen
        int local_idx = -1;
        for (int i = 0; i < cg->local_count; i++) {
            if (strcmp(cg->locals[i].name, node->as.var_decl_node.name) == 0) {
                local_idx = cg->locals[i].index;
                break;
            }
        }

        if (local_idx == -1) {
            if (cg->local_count >= 256) {
                fprintf(stderr, "[Foxy Codegen Error] Tabla de variables locales llena (max 256)\n");
                return false;
            }
            local_idx = cg->local_count;
            strncpy(cg->locals[cg->local_count].name, node->as.var_decl_node.name, 63);
            cg->locals[cg->local_count].name[63] = '\0';
            cg->locals[cg->local_count].index = local_idx;
            cg->local_count++;
        }

        // 3. DESPUÉS: Emitir FOXCODE_STORE_LOCAL (local_idx va en el argumento A)
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_ASSIGN: {
        if (!f_codegen_visit(cg, node->as.assign_node.value)) return false;

        const char *var_name = node->as.assign_node.name;
        int local_idx = f_codegen_resolve_local(cg, var_name);

        f_codegen_emit(cg, CREATE_ABC(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_IF: {
        if (!f_codegen_visit(cg, node->as.if_node.condition)) return false;

        size_t then_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));

        if (!f_codegen_visit(cg, node->as.if_node.then_branch)) return false;

        if (node->as.if_node.else_branch) {
            size_t else_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, 0));

            size_t else_start = cg->code_count;
            cg->bytecode[then_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)else_start);

            if (!f_codegen_visit(cg, node->as.if_node.else_branch)) return false;

            size_t if_end = cg->code_count;
            cg->bytecode[else_jump] = CREATE_ABx(FOXCODE_JUMP, 0, (uint16_t)if_end);
        } else {
            size_t if_end = cg->code_count;
            cg->bytecode[then_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)if_end);
        }
        return true;
    }

    lbl_FOXY_AST_NODE_WHILE: {
        size_t loop_start = cg->code_count;

        if (!f_codegen_visit(cg, node->as.while_node.condition)) return false;

        size_t exit_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));

        if (node->as.while_node.body) {
            if (!f_codegen_visit(cg, node->as.while_node.body)) return false;
        }

        f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, (uint16_t)loop_start));

        size_t loop_end = cg->code_count;
        cg->bytecode[exit_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)loop_end);
        return true;
    }

    lbl_FOXY_AST_NODE_RETURN: {
        if (node->as.return_node.value) {
            if (!f_codegen_visit(cg, node->as.return_node.value)) return false;
        } else {
            f_codegen_emit_byte(cg, FOXCODE_LOAD_NULL);
        }
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_FOR: {
        if (node->as.for_node.init) {
            if (!f_codegen_visit(cg, node->as.for_node.init)) return false;
        }

        size_t loop_start = cg->code_count;
        size_t exit_jump = (size_t)-1;

        if (node->as.for_node.condition) {
            if (!f_codegen_visit(cg, node->as.for_node.condition)) return false;
            exit_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));
        }

        if (node->as.for_node.body) {
            if (!f_codegen_visit(cg, node->as.for_node.body)) return false;
        }

        if (node->as.for_node.increment) {
            if (!f_codegen_visit(cg, node->as.for_node.increment)) return false;
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0));
        }

        f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, (uint16_t)loop_start));

        if (exit_jump != (size_t)-1) {
            size_t loop_end = cg->code_count;
            cg->bytecode[exit_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)loop_end);
        }
        return true;
    }

    lbl_FOXY_AST_NODE_ENV: {
        f_codegen_emit_env(cg);
        return true;
    }

    lbl_FOXY_AST_NODE_ENV_CREATE: {
        f_codegen_visit_env_create(cg, node);
        return true;
    }

    lbl_FOXY_AST_NODE_ENV_BIND: {
        f_codegen_visit_env_bind(cg, node);
        return true;
    }

    lbl_FOXY_AST_NODE_POPEN: {
        f_codegen_visit_popen(cg, node);
        return true;
    }

    lbl_FOXY_AST_NODE_DEFAULT: {
        fprintf(stderr, "[Foxy Codegen Warning] Nodo AST no soportado: %s\n", 
            f_ast_node_type_to_string(node->type));
        return true;
    }
}

void f_codegen_emit_null(FoxyCodegen *cg) {
    if (!cg) return;
    
    FoxyValue null_val = { .type = FOXY_VAL_NULL, .as.ptr = NULL };
    int const_idx = f_codegen_add_constant(cg, null_val);
    if (const_idx >= 0) {
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
    }
}

void f_codegen_emit_env(FoxyCodegen *cg) {
    if (!cg) return;
    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV, 0, 0, 0));
}

void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.env_create_node.name_expr != NULL) {
        f_codegen_visit(cg, node->as.env_create_node.name_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_CREATE, 0, 0, 0));
}

void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.env_bind_node.process_expr != NULL) {
        f_codegen_visit(cg, node->as.env_bind_node.process_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    if (node->as.env_bind_node.env_expr != NULL) {
        f_codegen_visit(cg, node->as.env_bind_node.env_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_BIND, 0, 0, 0));
}

void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.popen_node.callback_expr != NULL) {
        f_codegen_visit(cg, node->as.popen_node.callback_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    if (node->as.popen_node.name_expr != NULL) {
        f_codegen_visit(cg, node->as.popen_node.name_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    if (node->as.popen_node.env_expr != NULL) {
        f_codegen_visit(cg, node->as.popen_node.env_expr);
    } else {
        f_codegen_emit_null(cg);
    }

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_POPEN, 0, 0, 0));
}

bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root) {
    if (!cg || !ast_root) return false;

    if (!f_codegen_visit(cg, ast_root)) {
        return false;
    }

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));
    return true;
}