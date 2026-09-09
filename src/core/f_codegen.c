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
#include "f_vm.h"
// #include "utarray.h"

// static const UT_icd fox_instruction_icd = {sizeof(FoxInstruction), NULL, NULL, NULL};
// static const UT_icd foxy_value_icd = {sizeof(FoxyValue), NULL, NULL, NULL};

void f_codegen_init(FoxyCodegen *cg) {
    if (!cg) return;

    // memset debe ir al inicio ANTES de asignar memoria
    memset(cg, 0, sizeof(FoxyCodegen));

    cg->code_count = 0;
    cg->code_capacity = 8;
    cg->bytecode = (FoxInstruction*)malloc(sizeof(FoxInstruction) * cg->code_capacity);

    cg->constants_count = 0;
    cg->constants_capacity = 8;
    cg->constants = (FoxyValue*)malloc(sizeof(FoxyValue) * cg->constants_capacity);

    cg->locals_count = 0;
    cg->scope_depth = 0;
    cg->parent = NULL;
}

FoxyCodegen* f_codegen_create(void) {
    FoxyCodegen *cg = (FoxyCodegen*)malloc(sizeof(FoxyCodegen));
    if (!cg) return NULL;
    f_codegen_init(cg);
    return cg;
}

void f_codegen_free(FoxyCodegen *cg, FoxyVM *vm) {
    if (!cg) return;

    // Liberar el buffer de bytecode si no se ha liberado antes
    if (cg->bytecode) {
        free(cg->bytecode);
        cg->bytecode = NULL;
    }

    // Liberar el pool de constantes únicamente si no se transfirió la propiedad a la VM
    if (cg->constants) {
        for (size_t i = 0; i < cg->constants_count; i++) {
            f_value_free_contents(&cg->constants[i], vm);
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

size_t f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val, FoxyVM *vm) {
    if (!cg) return 0;

    // Deduplicación para arreglos / cadenas de texto
    if (val.type == FOXY_VAL_ARRAY || val.type == FOXY_VAL_OBJECT) {
        const char *str_new = f_value_get_char_array_data(&val);

        for (size_t i = 0; i < cg->constants_count; i++) {
            FoxyValue *existing = &cg->constants[i];
            if (existing->type == val.type) {
                const char *str_exist = f_value_get_char_array_data(existing);
                if (str_new && str_exist && strcmp(str_new, str_exist) == 0) {
                    f_value_free_contents(&val, vm);
                    return i;
                }
            }
        }
    } else {
        for (size_t i = 0; i < cg->constants_count; i++) {
            if (f_value_equals(&cg->constants[i], &val)) {
                return i;
            }
        }
    }

    if (cg->constants_count >= cg->constants_capacity) {
        size_t new_cap = cg->constants_capacity == 0 ? 8 : cg->constants_capacity * 2;
        cg->constants = (FoxyValue*)realloc(cg->constants, sizeof(FoxyValue) * new_cap);
        cg->constants_capacity = new_cap;
    }

    // Almacenar el valor en la tabla local del generador
    cg->constants[cg->constants_count] = val;

    if (vm && (val.type == FOXY_VAL_ARRAY || val.type == FOXY_VAL_OBJECT)) {
        f_codegen_add_char_array_constant(vm, f_value_get_char_array_data(&val));
    }

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

void f_codegen_emit_null(FoxyCodegen *cg, FoxyVM *vm) {
    if (!cg) return;
    
    FoxyValue null_val = {0};
    null_val.type = FOXY_VAL_NULL;
    null_val.as.ptr = NULL;
    int const_idx = (int)f_codegen_add_constant(cg, null_val, vm);
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

void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node, FoxyVM *vm) {
    if (!cg || !node) return;

    if (node->as.env_create_node.name_expr != NULL)
        f_codegen_visit(cg, node->as.env_create_node.name_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_CREATE, 0, 0, 0));
}

void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node, FoxyVM *vm) {
    if (!cg || !node) return;

    if (node->as.env_bind_node.process_expr != NULL)
        f_codegen_visit(cg, node->as.env_bind_node.process_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    if (node->as.env_bind_node.env_expr != NULL)
        f_codegen_visit(cg, node->as.env_bind_node.env_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_ENV_BIND, 0, 0, 0));
}

void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node, FoxyVM *vm) {
    if (!cg || !node) return;

    if (node->as.popen_node.callback_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.callback_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    if (node->as.popen_node.name_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.name_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    if (node->as.popen_node.env_expr != NULL)
        f_codegen_visit(cg, node->as.popen_node.env_expr, vm);
    else
        f_codegen_emit_null(cg, vm);

    f_codegen_emit(cg, CREATE_ABC(FOXCODE_POPEN, 0, 0, 0));
}

bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root, FoxyVM *vm) {
    if (!cg || !ast_root) return false;
    if (!f_codegen_visit(cg, ast_root, vm)) return false;
    f_codegen_emit(cg, CREATE_ABC(FOXCODE_HALT, 0, 0, 0));
    return true;
}

bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node, FoxyVM *vm) {
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
            if (!f_codegen_visit(cg, node->as.program_node.statements[i], vm)) return false;
        }
        return true;
    }

    lbl_FOXY_AST_NODE_INCLUDE: {
        const char *path_str = node->as.include_node.path;
        FoxyValue path_val = f_value_create_char_array(path_str, strlen(path_str));
        int const_idx = (int)f_codegen_add_constant(cg, path_val, vm);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_INCLUDE, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_EXPR_STMT: {
        if (node->as.expr_stmt_node.expression) {
            if (!f_codegen_visit(cg, node->as.expr_stmt_node.expression, vm)) return false;
            f_codegen_emit(cg, CREATE_ABC(FOXCODE_POP, 1, 0, 0));
        }
        return true;
    }

    lbl_FOXY_AST_NODE_CALL: {
        // 1. Visit and evaluate all arguments onto the stack in order
        for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
            if (!f_codegen_visit(cg, node->as.call_node.arguments[i], vm)) return false;
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
            int const_idx = f_codegen_add_constant(cg, sym_val, vm);
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_GLOBAL, 0, (uint16_t)const_idx));
        }

        // 3. Emit call instruction with argument count
        int arg_count = (int)node->as.call_node.arg_count;
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_CALL, (uint8_t)arg_count, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_LITERAL: {
        // Acomodar la constante en el pool asegurando que node->as.literal_node.value sea FoxyValue válido
        int const_idx = (int)f_codegen_add_constant(cg, node->as.literal_node.value, vm);
        if (const_idx < 0) return false;

        // Cargar constante hacia el tope de pila / registro activo actual
        uint8_t target_reg = 0; // O el índice de registro asignado si manejas asignación de registros
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, target_reg, (uint16_t)const_idx));
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
                if (!f_codegen_visit(cg, node->as.block_node.statements[i], vm)) return false;
            }
        }
        return true;
    }

    lbl_FOXY_AST_NODE_BINARY_OP: {
        if (!f_codegen_visit(cg, node->as.binary_node.left, vm)) return false;
        if (!f_codegen_visit(cg, node->as.binary_node.right, vm)) return false;

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
        // 1. Resolver o agregar la variable local primero para obtener su índice de registro
        int local_idx = -1;
        for (uint i = 0; i < cg->locals_count; i++) {
            if (strcmp(cg->locals[i].name, node->as.var_decl_node.name) == 0) {
                local_idx = cg->locals[i].index;
                break;
            }
        }

        if (local_idx == -1) {
            if ((uint)cg->locals_count >= FOXY_MAX_LOCALS) {
                fprintf(stderr, "[Foxy Codegen Error] Tabla de variables locales llena\n");
                return false;
            }
            local_idx = cg->locals_count;
            strncpy(cg->locals[cg->locals_count].name, node->as.var_decl_node.name, FOXY_MAX_IDENTIFIER_LEN - 1);
            cg->locals[cg->locals_count].name[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
            cg->locals[cg->locals_count].index = local_idx;
            cg->locals_count++;
        }

        // 2. Evaluar inicializador
        if (node->as.var_decl_node.initializer) {
            if (!f_codegen_visit(cg, node->as.var_decl_node.initializer, vm)) return false;
        } else {
            FoxyValue null_val = { .type = FOXY_VAL_NULL, .as.ptr = NULL };
            int const_idx = (int)f_codegen_add_constant(cg, null_val, vm);
            if (const_idx < 0) return false;
            f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, (uint8_t)local_idx, (uint16_t)const_idx));
        }

        // 3. Almacenar el valor en la variable local
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_ASSIGN: {
        if (!f_codegen_visit(cg, node->as.assign_node.value, vm)) return false;
        const char *var_name = node->as.assign_node.name;
        int local_idx = f_codegen_resolve_local(cg, var_name);
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_STORE_LOCAL, (uint8_t)local_idx, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_IF: {
        if (!f_codegen_visit(cg, node->as.if_node.condition, vm)) return false;
        size_t then_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));
        if (!f_codegen_visit(cg, node->as.if_node.then_branch, vm)) return false;

        if (node->as.if_node.else_branch) {
            size_t else_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, 0));
            size_t else_start = cg->code_count;
            cg->bytecode[then_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)else_start);
            if (!f_codegen_visit(cg, node->as.if_node.else_branch, vm)) return false;
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
        if (!f_codegen_visit(cg, node->as.while_node.condition, vm)) return false;
        size_t exit_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));
        if (node->as.while_node.body) {
            if (!f_codegen_visit(cg, node->as.while_node.body, vm)) return false;
        }
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP, 0, (uint16_t)loop_start));
        size_t loop_end = cg->code_count;
        cg->bytecode[exit_jump] = CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, (uint16_t)loop_end);
        return true;
    }

    lbl_FOXY_AST_NODE_FUNCTION: {
        FoxyCodegen func_cg;
        f_codegen_init(&func_cg);

        // 1. Agregar parámetros a la tabla local
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

        // 2. Visitar el cuerpo de la función
        if (node->as.function_node.body) {
            if (!f_codegen_visit(&func_cg, node->as.function_node.body, vm)) {
                f_codegen_free(&func_cg, vm);
                return false;
            }
        }

        // Emitir RET explícito al final de la función ANTES de empaquetar
        f_codegen_emit(&func_cg, CREATE_ABC(FOXCODE_RET, 0, 0, 0));

        // 3. Instanciar FoxyFunction
        FoxyFunction *fn = (FoxyFunction*)calloc(1, sizeof(FoxyFunction));
        if (!fn) {
            fprintf(stderr, "[Foxy Codegen Error] Memory allocation failed for FoxyFunction\n");
            f_codegen_free(&func_cg, vm);
            return false;
        }

        fn->name = node->as.function_node.name ? strdup(node->as.function_node.name) : NULL;
        fn->arity = (uint8_t)node->as.function_node.param_count;
        fn->type = FOXY_FUNCTION_USER;

        // Transferir directamente los recursos
        fn->as.user.code = func_cg.bytecode;
        fn->as.user.code_size = func_cg.code_count * sizeof(FoxInstruction);
        fn->as.user.code_capacity = func_cg.code_capacity;

        fn->as.user.constants = func_cg.constants;
        fn->as.user.constants_count = func_cg.constants_count;
        fn->as.user.constants_capacity = func_cg.constants_capacity;

        fn->as.user.locals_count = func_cg.locals_count;
        fn->as.user.locals_capacity = FOXY_MAX_LOCALS;
        fn->as.user.env = NULL;

        // 4. Agregar la función al pool del generador padre
        FoxyValue func_val = {0};
        func_val.type = FOXY_VAL_FUNCTION;
        func_val.as.func = fn;

        int const_idx = (int)f_codegen_add_constant(cg, func_val, vm);
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_LOAD_CONST, 0, (uint16_t)const_idx));

        // 5. Registrar el símbolo local en el generador padre
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
                    fprintf(stderr, "[Foxy Codegen Error] Local variable table full for '%s'\n", node->as.function_node.name);
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
            if (!f_codegen_visit(cg, node->as.return_node.value, vm)) return false;
        } else {
            f_codegen_emit_null(cg, vm);
        }
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_RET, 0, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_FOR: {
        if (node->as.for_node.init) {
            if (!f_codegen_visit(cg, node->as.for_node.init, vm)) return false;
        }

        size_t loop_start = cg->code_count;
        size_t exit_jump = (size_t)-1;

        if (node->as.for_node.condition) {
            if (!f_codegen_visit(cg, node->as.for_node.condition, vm)) return false;
            exit_jump = f_codegen_emit(cg, CREATE_ABx(FOXCODE_JUMP_IF_FALSE, 0, 0));
        }

        if (node->as.for_node.body) {
            if (!f_codegen_visit(cg, node->as.for_node.body, vm)) return false;
        }

        if (node->as.for_node.increment) {
            if (!f_codegen_visit(cg, node->as.for_node.increment, vm)) return false;
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
        f_codegen_visit_env_create(cg, node, vm);
        return true;
    }

    lbl_FOXY_AST_NODE_ENV_BIND: {
        f_codegen_visit_env_bind(cg, node, vm);
        return true;
    }

    lbl_FOXY_AST_NODE_POPEN: {
        f_codegen_visit_popen(cg, node, vm);
        return true;
    }

    lbl_FOXY_AST_NODE_MEMBER_ACCESS: {
        if (!f_codegen_visit(cg, node->as.member_access_node.target, vm)) return false;
        
        FoxyValue field_val = f_value_create_char_array(
            node->as.member_access_node.field,
            strlen(node->as.member_access_node.field)
        );
        int const_idx = (int)f_codegen_add_constant(cg, field_val, vm);
        
        // Usar FOXCODE_GET_INDEX o la instrucción definida en tu f_foxmode.h / f_bytecode.h
        f_codegen_emit(cg, CREATE_ABx(FOXCODE_GET_INDEX, 0, (uint16_t)const_idx));
        return true;
    }

    lbl_FOXY_AST_NODE_INDEX_ACCESS: {
        // 1. Evaluar el contenedor (target) y el índice (index)
        if (!f_codegen_visit(cg, node->as.index_access_node.target, vm)) return false;
        if (!f_codegen_visit(cg, node->as.index_access_node.index, vm)) return false;

        // 2. Emitir instrucción de acceso por índice
        f_codegen_emit(cg, CREATE_ABC(FOXCODE_GET_INDEX, 0, 0, 0));
        return true;
    }

    lbl_FOXY_AST_NODE_DEFAULT: {
        fprintf(stderr, "[Foxy Codegen Warning] Nodo AST no soportado: %s\n", 
            f_ast_node_type_to_string(node->type));
        return true;
    }
}

size_t f_codegen_add_char_array_constant(FoxyVM *vm, const char *text) {
    if (!vm || !text) return (size_t)-1;

    // Redimensionar el pool de constantes si se alcanzó la capacidad máxima
    if (vm->constants_count >= vm->constants_capacity) {
        size_t new_cap = vm->constants_capacity == 0 ? 8 : vm->constants_capacity * 2;
        FoxyValue *new_constants = (FoxyValue *)realloc(vm->constants, sizeof(FoxyValue) * new_cap);
        if (!new_constants) return (size_t)-1;
        
        vm->constants = new_constants;
        vm->constants_capacity = new_cap;
    }

    size_t idx = vm->constants_count;

    // Empaquetar el literal char[] usando el constructor nativo de f_value
    vm->constants[idx] = f_value_create_char_array(text, strlen(text));
    vm->constants_count++;

    return idx;
}