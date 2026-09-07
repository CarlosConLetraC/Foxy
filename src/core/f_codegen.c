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

void f_codegen_init(FoxyCodegen *cg) {
    if (!cg) return;
    
    cg->code_count = 0;
    cg->code_capacity = FOXY_MAX_CODE_CAPACITY;
    f_array_init(cg->bytecode, cg->code_capacity);

    cg->constants_count = 0;
    cg->constants_capacity = FOXY_MAX_CONSTANTS_CAPACITY;
    f_array_init(cg->constants, cg->constants_capacity);

    cg->local_count = 0;
}

void f_codegen_free(FoxyCodegen *cg) {
    if (!cg) return;

    // 1. Liberar el búfer de bytecode
    if (cg->bytecode) {
        free(cg->bytecode);
        cg->bytecode = NULL;
    }

    // 2. Liberar los arreglos, cadenas y funciones dentro del pool de constantes de forma segura
    if (cg->constants) {
        for (size_t i = 0; i < cg->constants_count; i++) {
            FoxyValue *val = &cg->constants[i]; // Definimos val para todo el ciclo

            // Manejamos de forma genérica cualquier tipo que contenga un arreglo dinámico en el heap
            if (val->type == FOXY_VAL_ARRAY || val->type == FOXY_VAL_OBJECT) {
                FoxyArray *arr = val->as.array;
                if (arr) {
                    if (arr->data) {
                        free(arr->data);
                        arr->data = NULL;
                    }
                    free(arr);
                    val->as.array = NULL;
                }
            } else if (val->type == FOXY_VAL_CHAR) {
                if (val->as.sval) {
                    free((void *)val->as.sval); // Cast a void* para descartar el const de forma segura
                    val->as.sval = NULL;
                }
            } else if (val->type == FOXY_VAL_FUNCTION) {
                f_function_free(val->as.func);
                val->as.func = NULL;
            }
        }
        free(cg->constants);
        cg->constants = NULL;
    }

    cg->code_count = 0;
    cg->code_capacity = 0;
    cg->constants_count = 0;
    cg->constants_capacity = 0;
}

size_t f_codegen_emit(FoxyCodegen *cg, FoxInstruction inst) {
    if (!cg) return 0;

    // Sustituye el realloc manual y la verificación de Out of Memory
    f_array_push(cg->bytecode, cg->code_count, cg->code_capacity, inst);
    return cg->code_count - 1;
}

void f_codegen_emit_byte(FoxyCodegen *cg, uint8_t opcode) {
    if (!cg) return;
    f_codegen_emit(cg, CREATE_ABC((FoxOpcode)opcode, 0, 0, 0));
}

size_t f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val) {
    if (val.type == FOXY_VAL_ARRAY && val.as.array) {
        FoxyArray *orig_arr = val.as.array;
        FoxyArray *new_arr = malloc(sizeof(FoxyArray));
        
        if (new_arr) {
            new_arr->element_type_id = orig_arr->element_type_id;
            new_arr->length = orig_arr->length;
            
            if (orig_arr->data && orig_arr->length > 0) {
                new_arr->data = malloc(orig_arr->length + 1);
                if (new_arr->data) {
                    memcpy(new_arr->data, orig_arr->data, orig_arr->length);
                    ((char *)new_arr->data)[orig_arr->length] = '\0';
                }
            } else {
                new_arr->data = NULL;
            }
            
            val.as.array = new_arr;
        }
    }

    // Inserción segura con auto-expansión vía f_array
    f_array_push(cg->constants, cg->constants_count, cg->constants_capacity, val);
    return cg->constants_count - 1;
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
        int const_idx = f_codegen_add_constant(cg, path_val);

        // f_codegen_add_constant clona o toma posesión. 
        // Liberamos el contenedor temporal path_val para no dejar bloques huérfanos
        f_value_free_contents(&path_val);

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
        for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
            if (!f_codegen_visit(cg, node->as.call_node.arguments[i])) return false;
        }

        const char *callee = node->as.call_node.callee_name;
        int local_idx = f_codegen_resolve_local(cg, callee);

        f_codegen_emit(cg, CREATE_ABC(FOXCODE_LOAD_LOCAL, (uint8_t)local_idx, 0, 0));

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
            FoxyValue null_val = { .type = FOXY_VAL_NULL, .as.ptr = NULL };
            int const_idx = f_codegen_add_constant(cg, null_val);
            if (const_idx < 0) return false;
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));
        }

        int local_idx = -1;
        for (uint i = 0; i < cg->local_count; i++) {
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
            strncpy(cg->locals[cg->local_count].name, node->as.var_decl_node.name, FOXY_MAX_IDENTIFIER_LEN - 1);
            cg->locals[cg->local_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
            cg->locals[cg->local_count].index = local_idx;
            cg->local_count++;
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

        for (size_t i = 0; i < node->as.function_node.param_count; i++) {
            if (func_cg.local_count < FOXY_MAX_LOCALS) {
                strncpy(
                    func_cg.locals[func_cg.local_count].name, 
                    node->as.function_node.param_names[i], 
                    FOXY_MAX_IDENTIFIER_LEN - 1
                );
                func_cg.locals[func_cg.local_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
                func_cg.locals[func_cg.local_count].index = func_cg.local_count;
                func_cg.local_count++;
            }
        }

        if (node->as.function_node.body) {
            if (!f_codegen_visit(&func_cg, node->as.function_node.body)) {
                f_codegen_free(&func_cg);
                return false;
            }
        }

        f_codegen_emit(&func_cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));

        FoxyFunction *fn = malloc(sizeof(FoxyFunction));
        if (!fn) {
            fprintf(stderr, "[Foxy Codegen Error] Out of memory allocating FoxyFunction\n");
            f_codegen_free(&func_cg);
            return false;
        }

        fn->name = node->as.function_node.name ? strdup(node->as.function_node.name) : NULL;
        fn->arity = (uint8_t)node->as.function_node.param_count;
        fn->type = FOXY_FUNCTION_USER;

        fn->as.user.code_size = func_cg.code_count * sizeof(FoxInstruction);
        fn->as.user.code = malloc(fn->as.user.code_size);
        if (!fn->as.user.code && fn->as.user.code_size > 0) {
            fprintf(stderr, "[Foxy Codegen Error] Out of memory allocating function code buffer\n");
            if (fn->name) free(fn->name);
            free(fn);
            f_codegen_free(&func_cg);
            return false;
        }
        memcpy(fn->as.user.code, func_cg.bytecode, fn->as.user.code_size);
        fn->as.user.code_capacity = func_cg.code_capacity;

        fn->as.user.locals_count = func_cg.local_count;
        fn->as.user.locals_capacity = FOXY_MAX_LOCALS;

        fn->as.user.constants = func_cg.constants;
        fn->as.user.constants_count = func_cg.constants_count;
        fn->as.user.constants_capacity = func_cg.constants_capacity;
        fn->as.user.env = NULL;

        free(func_cg.bytecode);

        FoxyValue func_val = {0};
        func_val.type = FOXY_VAL_FUNCTION;
        func_val.as.func = fn;

        int const_idx = f_codegen_add_constant(cg, func_val);
        
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));

        if (node->as.function_node.name) {
            int local_idx = -1;
            for (uint i = 0; i < cg->local_count; i++) {
                if (strcmp(cg->locals[i].name, node->as.function_node.name) == 0) {
                    local_idx = cg->locals[i].index;
                    break;
                }
            }

            if (local_idx == -1) {
                if (cg->local_count >= FOXY_MAX_LOCALS) {
                    fprintf(stderr, "[Foxy Codegen Error] Tabla de variables locales llena para la función '%s'\n", node->as.function_node.name);
                    f_function_free(fn);
                    return false;
                }
                local_idx = cg->local_count;
                strncpy(cg->locals[cg->local_count].name, node->as.function_node.name, FOXY_MAX_IDENTIFIER_LEN - 1);
                cg->locals[cg->local_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
                cg->locals[cg->local_count].index = local_idx;
                cg->local_count++;
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