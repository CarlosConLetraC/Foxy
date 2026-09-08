#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_codegen.h"
#include "f_parser.h"
#include "f_foxcode.h"
#include "f_value.h"
#include "f_class.h"
#include "f_dict.h"
#include "f_function.h"
#include "f_object.h"
#include "f_ast.h"
#include "f_array.h"
// #include "utarray.h"

// static const UT_icd fox_instruction_icd = {sizeof(FoxInstruction), NULL, NULL, NULL};
// static const UT_icd foxy_value_icd = {sizeof(FoxyValue), NULL, NULL, NULL};

void f_codegen_init(FoxyCodegen *cg) {
    if (!cg) return;

    cg->code_count = 0;
    cg->code_capacity = 8;
    cg->bytecode = (FoxInstruction*)malloc(sizeof(FoxInstruction) * cg->code_capacity);

    cg->constants_count = 0;
    cg->constants_capacity = 8;
    cg->constants = (FoxyValue*)malloc(sizeof(FoxyValue) * cg->constants_capacity);

    cg->locals_count = 0;
    cg->scope_depth = 0;
    cg->parent = NULL;
    memset(cg, 0, sizeof(FoxyCodegen));
}

FoxyCodegen* f_codegen_create(void) {
    FoxyCodegen *cg = (FoxyCodegen*)malloc(sizeof(FoxyCodegen));
    if (!cg) return NULL;
    f_codegen_init(cg);
    return cg;
}

void f_codegen_free(FoxyCodegen *cg) {
    if (!cg) return;

    // Liberar el buffer de bytecode si no se ha liberado antes
    if (cg->bytecode) {
        free(cg->bytecode);
        cg->bytecode = NULL;
    }

    // Liberar el pool de constantes únicamente si no se transfirió la propiedad a la VM
    if (cg->constants) {
        for (size_t i = 0; i < cg->constants_count; i++) {
            f_value_free_contents(&cg->constants[i]);
        }
        free(cg->constants);
        cg->constants = NULL;
    }

    cg->code_count = 0;
    cg->code_capacity = 0;
    cg->constants_count = 0;
    cg->constants_capacity = 0;
    cg->locals_count = 0;
}

size_t f_codegen_emit(FoxyCodegen *cg, FoxInstruction inst) {
    if (!cg) return 0;

    if (cg->code_count + 1 > cg->code_capacity) {
        size_t old_cap = cg->code_capacity;
        cg->code_capacity = FOXY_GROW_CAPACITY(old_cap);
        cg->bytecode = FOXY_GROW_ARRAY(FoxInstruction, cg->bytecode, old_cap, cg->code_capacity);
    }

    cg->bytecode[cg->code_count] = inst;
    return cg->code_count++;
}

size_t f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val) {
    // Si el valor es una cadena de caracteres, buscamos si ya existe
    if (val.type == FOXY_VAL_ARRAY || val.type == FOXY_VAL_OBJECT) {
        const char *str_new = f_value_get_char_array_data(&val);

        for (size_t i = 0; i < cg->constants_count; i++) {
            FoxyValue *existing = &cg->constants[i];
            if (existing->type == val.type) {
                const char *str_exist = f_value_get_char_array_data(existing);
                if (str_new && str_exist && strcmp(str_new, str_exist) == 0) {
                    // La cadena ya existe en la tabla de constantes.
                    // Liberamos el FoxyValue duplicado recien creado en el AST
                    // para evitar fugas de memoria y retornamos el índice existente.
                    f_value_free_contents(&val);
                    return i;
                }
            }
        }
    } else {
        // Mismalogica para primitivos (int, double, char) si deseas deduplicar todo
        for (size_t i = 0; i < cg->constants_count; i++) {
            if (f_value_equals(&cg->constants[i], &val)) {
                return i;
            }
        }
    }

    // Si no existe, se agrega como una nueva constante
    if (cg->constants_count >= cg->constants_capacity) {
        size_t new_cap = cg->constants_capacity == 0 ? 8 : cg->constants_capacity * 2;
        cg->constants = realloc(cg->constants, sizeof(FoxyValue) * new_cap);
        cg->constants_capacity = new_cap;
    }

    cg->constants[cg->constants_count] = val;
    return cg->constants_count++;
}

int f_codegen_resolve_local(FoxyCodegen *cg, const char *name) {
    if (!cg || !name) return -1;

    // Buscar si la variable ya existe en el scope actual o superior
    for (int i = (int)cg->locals_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) return i;
    }

    // Si no existe, se registra mediante f_codegen_add_local
    return f_codegen_add_local(cg, name, strlen(name));
}

void f_codegen_emit_byte(FoxyCodegen *cg, uint8_t opcode) {
    if (!cg) return;
    f_codegen_emit(cg, CREATE_ABC((FoxOpcode)opcode, 0, 0, 0));
}

void f_codegen_emit_null(FoxyCodegen *cg) {
    if (!cg) return;
    
    FoxyValue null_val = {0};
    null_val.type = FOXY_VAL_NULL;
    null_val.as.ptr = NULL;
    int const_idx = (int)f_codegen_add_constant(cg, null_val);
    if (const_idx >= 0) f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
}

void f_codegen_emit_env(FoxyCodegen *cg) {
    if (!cg) return;
    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV, 0, 0, 0));
}

int f_codegen_add_local(FoxyCodegen *cg, const char *name, size_t name_len) {
    if (!cg || !name || name_len == 0) {
        return -1;
    }

    // Guard de desbordamiento estricto
    if (cg->locals_count >= FOXY_MAX_LOCALS) {
        fprintf(stderr, "[Error Codegen] Límite de variables locales alcanzado (%d).\n", FOXY_MAX_LOCALS);
        return -1;
    }

    size_t copy_len = name_len < (FOXY_MAX_IDENTIFIER_LEN - 1) ? name_len : (FOXY_MAX_IDENTIFIER_LEN - 1);

    FoxyCodegenLocal *local = &cg->locals[cg->locals_count];
    memset(local, 0, sizeof(FoxyCodegenLocal));

    memcpy(local->name, name, copy_len);
    local->name[copy_len] = '\0';
    local->depth = cg->scope_depth;
    local->is_captured = false;
    local->index = (int)cg->locals_count;

    return (int)cg->locals_count++;
}

void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.env_create_node.name_expr != NULL)
        f_codegen_visit(cg, node->as.env_create_node.name_expr);
    else
        f_codegen_emit_null(cg);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_CREATE, 0, 0, 0));
}

void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.env_bind_node.process_expr != NULL)
        f_codegen_visit(cg, node->as.env_bind_node.process_expr);
    else
        f_codegen_emit_null(cg);

    if (node->as.env_bind_node.env_expr != NULL)
        f_codegen_visit(cg, node->as.env_bind_node.env_expr);
    else
        f_codegen_emit_null(cg);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_BIND, 0, 0, 0));
}

void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!cg || !node) return;

    if (node->as.popen_node.callback_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.callback_expr);
    else
        f_codegen_emit_null(cg);

    if (node->as.popen_node.name_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.name_expr);
    else
        f_codegen_emit_null(cg);

    if (node->as.popen_node.env_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.env_expr);
    else
        f_codegen_emit_null(cg);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_POPEN, 0, 0, 0));
}

bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root) {
    if (!cg || !ast_root) return false;
    if (!f_codegen_visit(cg, ast_root)) return false;
    f_codegen_emit(cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));
    return true;
}

bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node) {
    if (!node || !cg) return false;

    #define MAKE_LABEL_PTR(enum_val, name_str) [enum_val] = &&lbl_##enum_val,
    static const void *dispatch_table[] = {
        FOXY_AST_NODE_LIST(MAKE_LABEL_PTR)
    };
    #undef MAKE_LABEL_PTR

    if ((size_t)node->type >= sizeof(dispatch_table) / sizeof(dispatch_table[0]) || !dispatch_table[node->type])
        goto lbl_FOXY_AST_NODE_DEFAULT;

    goto *dispatch_table[node->type];

    lbl_FOXY_AST_NODE_PROGRAM: {
        for (size_t i = 0; i < node->as.program_node.count; i++) {
            if (!f_codegen_visit(cg, node->as.program_node.statements[i])) return false;
        }
        return true;
    }

    lbl_FOXY_AST_NODE_INCLUDE: {
        const char *path_str = node->as.include_node.path;
        FoxyValue path_val = f_value_create_char_array(path_str, strlen(path_str));
        int const_idx = (int)f_codegen_add_constant(cg, path_val);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_INCLUDE, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_EXPR_STMT: {
        if (node->as.expr_stmt_node.expression) {
            if (!f_codegen_visit(cg, node->as.expr_stmt_node.expression)) return false;
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0));
        }
        return true;
    }

    lbl_FOXY_AST_NODE_CALL: {
        // 1. Visit and evaluate all arguments onto the stack in order
        for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
            if (!f_codegen_visit(cg, node->as.call_node.arguments[i])) return false;
        }

        // 2. Resolve target function name
        const char *callee = node->as.call_node.callee_name;
        int local_idx = -1;

        for (int i = (int)cg->locals_count - 1; i >= 0; i--) {
            if (strcmp(cg->locals[i].name, callee) == 0) {
                local_idx = i;
                break;
            }
        }

        if (local_idx >= 0) {
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_LOAD_LOCAL, (uint8_t)local_idx, 0, 0));
        } else {
            FoxyValue sym_val = f_value_create_char_array(callee, strlen(callee));
            int const_idx = f_codegen_add_constant(cg, sym_val);
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_GLOBAL, 0, (uint16_t)const_idx));
        }

        // 3. Emit call instruction with argument count
        int arg_count = (int)node->as.call_node.arg_count;
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_CALL, (uint8_t)arg_count, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_LITERAL: {
        int const_idx = (int)f_codegen_add_constant(cg, node->as.literal_node.value);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_IDENTIFIER: {
        const char *id_name = node->as.identifier_node.name;
        int local_idx = f_codegen_resolve_local(cg, id_name);
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_LOAD_LOCAL, (uint8_t)local_idx, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_BLOCK: {
        if (node->as.block_node.statements) {
            for (size_t i = 0; i < node->as.block_node.count; i++) {
                if (!f_codegen_visit(cg, node->as.block_node.statements[i])) return false;
            }
        }
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
        if (node->as.var_decl_node.initializer) {
            if (!f_codegen_visit(cg, node->as.var_decl_node.initializer)) return false;
        } else {
            FoxyValue null_val = {0};
            null_val.type = FOXY_VAL_NULL;
            null_val.as.ptr = NULL;
            int const_idx = (int)f_codegen_add_constant(cg, null_val);
            if (const_idx < 0) return false;
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
        }

        int local_idx = -1;
        for (uint i = 0; i < cg->locals_count; i++) {
            if (strcmp(cg->locals[i].name, node->as.var_decl_node.name) == 0) {
                local_idx = cg->locals[i].index;
                break;
            }
        }

        if (local_idx == -1) {
            if ((uint)cg->locals_count >= FOXY_MAX_LOCALS) {
                fprintf(stderr, "[Foxy Codegen Error] Tabla de variables locales llena (max %u)\n", FOXY_MAX_LOCALS);
                return false;
            }
            local_idx = cg->locals_count;
            strncpy(cg->locals[cg->locals_count].name, node->as.var_decl_node.name, FOXY_MAX_IDENTIFIER_LEN - 1);
            cg->locals[cg->locals_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
            cg->locals[cg->locals_count].index = local_idx;
            cg->locals_count++;
        }

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

    lbl_FOXY_AST_NODE_FUNCTION: {
        FoxyCodegen func_cg;
        f_codegen_init(&func_cg);

        // 1. Agregar parámetros a la tabla local de la función
        for (size_t i = 0; i < node->as.function_node.param_count; i++) {
            if (func_cg.locals_count < FOXY_MAX_LOCALS) {
                strncpy(
                    func_cg.locals[func_cg.locals_count].name, 
                    node->as.function_node.param_names[i], 
                    FOXY_MAX_IDENTIFIER_LEN - 1
                );
                func_cg.locals[func_cg.locals_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
                func_cg.locals[func_cg.locals_count].index = func_cg.locals_count;
                func_cg.locals_count++;
            }
        }

        // 2. Visitar el cuerpo
        if (node->as.function_node.body) {
            if (!f_codegen_visit(&func_cg, node->as.function_node.body)) {
                f_codegen_free(&func_cg);
                return false;
            }
        }

        f_codegen_emit(&func_cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));

        // 3. Instanciar la estructura FoxyFunction
        FoxyFunction *fn = calloc(1, sizeof(FoxyFunction));
        if (!fn) {
            fprintf(stderr, "[Foxy Codegen Error] Out of memory allocating FoxyFunction\n");
            f_codegen_free(&func_cg);
            return false;
        }

        fn->name = node->as.function_node.name ? strdup(node->as.function_node.name) : NULL;
        fn->arity = (uint8_t)node->as.function_node.param_count;
        fn->type = FOXY_FUNCTION_USER;

        // Transferir directamente el bytecode en lugar de hacer malloc + memcpy + free
        fn->as.user.code = func_cg.bytecode;
        fn->as.user.code_size = func_cg.code_count * sizeof(FoxInstruction);
        fn->as.user.code_capacity = func_cg.code_capacity;

        // Transferir la tabla de constantes
        fn->as.user.constants = func_cg.constants;
        fn->as.user.constants_count = func_cg.constants_count;
        fn->as.user.constants_capacity = func_cg.constants_capacity;

        fn->as.user.locals_count = func_cg.locals_count;
        fn->as.user.locals_capacity = FOXY_MAX_LOCALS;
        fn->as.user.env = NULL;

        // IMPORTANTE: NO llamar a f_codegen_free(&func_cg) aquí.
        // Los buffers (bytecode y constants) ya pertenecen a 'fn'. 
        // Llamar a f_codegen_free(&func_cg) causaría double-free o invalid size.

        // 4. Agregar la función al pool de constantes del generador padre
        FoxyValue func_val = {0};
        func_val.type = FOXY_VAL_FUNCTION;
        func_val.as.func = fn;

        int const_idx = (int)f_codegen_add_constant(cg, func_val);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));

        // 5. Registrar el símbolo local
        if (node->as.function_node.name) {
            int local_idx = -1;
            for (uint i = 0; i < cg->locals_count; i++) {
                if (strcmp(cg->locals[i].name, node->as.function_node.name) == 0) {
                    local_idx = cg->locals[i].index;
                    break;
                }
            }

            if (local_idx == -1) {
                if (cg->locals_count >= FOXY_MAX_LOCALS) {
                    fprintf(stderr, "[Foxy Codegen Error] Tabla de variables locales llena para '%s'\n", node->as.function_node.name);
                    return false;
                }
                local_idx = cg->locals_count;
                strncpy(cg->locals[cg->locals_count].name, node->as.function_node.name, FOXY_MAX_IDENTIFIER_LEN - 1);
                cg->locals[cg->locals_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
                cg->locals[cg->locals_count].index = local_idx;
                cg->locals_count++;
            }

            f_codegen_emit(cg, CREATE_ABx(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0));
        }

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