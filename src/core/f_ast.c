#include "f_ast.h"
#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char * const FOXY_AST_NODE_TYPE_NAMES[] = {
#define F(node_type, name_str) name_str,
    FOXY_AST_NODE_LIST(F)
#undef F
};

const char* f_ast_node_type_to_string(FoxyASTNodeType type) {
    if (type >= AST_NODE_COUNT) {
        return "UNKNOWN";
    }
    return FOXY_AST_NODE_TYPE_NAMES[type];
}

FoxyASTNode* f_ast_node_new(FoxyASTNodeType type) {
    FoxyASTNode *node = (FoxyASTNode*)calloc(1, sizeof(FoxyASTNode));
    if (!node) {
        fprintf(stderr, "[Foxy AST Error] Sin memoria para crear nodo AST\n");
        return NULL;
    }
    node->type = type;
    return node;
}

void f_ast_node_free(FoxyASTNode *node, FoxyVM *vm) {
    if (!node) return;

    switch (node->type) {
        case FOXY_AST_NODE_PROGRAM:
            if (node->as.program_node.statements) {
                for (size_t i = 0; i < node->as.program_node.count; i++) {
                    f_ast_node_free(node->as.program_node.statements[i], vm);
                }
                free(node->as.program_node.statements);
            }
            break;
        case FOXY_AST_NODE_BLOCK:
            if (node->as.block_node.statements) {
                for (size_t i = 0; i < node->as.block_node.count; i++) {
                    f_ast_node_free(node->as.block_node.statements[i], vm);
                }
                free(node->as.block_node.statements);
            }
            break;
        case FOXY_AST_NODE_BINARY_OP:
        case FOXY_AST_NODE_ASSIGN:
            f_ast_node_free(node->as.binary_node.left, vm);
            f_ast_node_free(node->as.binary_node.right, vm);
            break;
        case FOXY_AST_NODE_LITERAL:
            f_value_free_contents(&node->as.literal_node.value, vm);
            break;
        case FOXY_AST_NODE_IDENTIFIER:
            if (node->as.identifier_node.name) free(node->as.identifier_node.name);
            break;
        case FOXY_AST_NODE_FUNCTION:
            if (node->as.function_node.name) free(node->as.function_node.name);
            if (node->as.function_node.param_names) {
                for (size_t i = 0; i < node->as.function_node.param_count; i++) {
                    free(node->as.function_node.param_names[i]);
                }
                free(node->as.function_node.param_names);
            }
            if (node->as.function_node.body) f_ast_node_free(node->as.function_node.body, vm);
            break;
        case FOXY_AST_NODE_RETURN:
            if (node->as.return_node.value) f_ast_node_free(node->as.return_node.value, vm);
            break;
        case FOXY_AST_NODE_VAR_DECL:
            if (node->as.var_decl_node.name) free(node->as.var_decl_node.name);
            if (node->as.var_decl_node.initializer) f_ast_node_free(node->as.var_decl_node.initializer, vm);
            break;
        case FOXY_AST_NODE_CALL:
            if (node->as.call_node.callee_name) free(node->as.call_node.callee_name);
            if (node->as.call_node.arguments) {
                for (size_t i = 0; i < node->as.call_node.arg_count; i++) {
                    f_ast_node_free(node->as.call_node.arguments[i], vm);
                }
                free(node->as.call_node.arguments);
            }
            break;
        default:
            break;
    }
    free(node);
}

FoxyASTNode* f_ast_create_program(void) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_PROGRAM);
    if (!node) return NULL;
    node->as.program_node.count = 0;
    node->as.program_node.capacity = 8;
    node->as.program_node.statements = (FoxyASTNode**)malloc(sizeof(FoxyASTNode*) * 8);
    return node;
}

void f_ast_program_add(FoxyASTNode *program, FoxyASTNode *stmt) {
    if (!program || !stmt || program->type != FOXY_AST_NODE_PROGRAM) return;

    if (program->as.program_node.count + 1 > program->as.program_node.capacity) {
        size_t old_cap = program->as.program_node.capacity;
        size_t new_cap = old_cap < 8 ? 8 : old_cap * 2;
        FoxyASTNode **new_stmts = (FoxyASTNode**)realloc(program->as.program_node.statements, sizeof(FoxyASTNode*) * new_cap);
        if (!new_stmts) return;
        program->as.program_node.statements = new_stmts;
        program->as.program_node.capacity = new_cap;
    }

    program->as.program_node.statements[program->as.program_node.count++] = stmt;
}

FoxyASTNode* f_ast_create_include(const char *path) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_INCLUDE);
    if (!node) return NULL;
    node->as.include_node.path = path ? strdup(path) : NULL;
    return node;
}

FoxyASTNode* f_ast_create_call(const char *callee) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_CALL);
    if (!node) return NULL;
    node->as.call_node.callee_name = callee ? strdup(callee) : NULL;
    node->as.call_node.arg_count = 0;
    node->as.call_node.arg_capacity = 4;
    node->as.call_node.arguments = (FoxyASTNode**)malloc(sizeof(FoxyASTNode*) * 4);
    return node;
}

void f_ast_call_add_arg(FoxyASTNode *call_node, FoxyASTNode *arg) {
    if (!call_node || !arg || call_node->type != FOXY_AST_NODE_CALL) return;

    if (call_node->as.call_node.arg_count + 1 > call_node->as.call_node.arg_capacity) {
        size_t old_cap = call_node->as.call_node.arg_capacity;
        size_t new_cap = old_cap < 4 ? 4 : old_cap * 2;
        FoxyASTNode **new_args = (FoxyASTNode**)realloc(call_node->as.call_node.arguments, sizeof(FoxyASTNode*) * new_cap);
        if (!new_args) return;
        call_node->as.call_node.arguments = new_args;
        call_node->as.call_node.arg_capacity = new_cap;
    }

    call_node->as.call_node.arguments[call_node->as.call_node.arg_count++] = arg;
}

FoxyASTNode* f_ast_create_literal(FoxyValue val) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_LITERAL);
    if (!node) return NULL;
    node->as.literal_node.value = val;
    return node;
}

FoxyASTNode* f_ast_create_identifier(const char *name) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
    if (!node) return NULL;
    node->as.identifier_node.name = name ? strdup(name) : NULL;
    return node;
}

FoxyASTNode* f_ast_create_binary_op(int op_token, FoxyASTNode *left, FoxyASTNode *right) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_BINARY_OP);
    if (!node) return NULL;
    node->as.binary_node.op_token = op_token;
    node->as.binary_node.left = left;
    node->as.binary_node.right = right;
    return node;
}

FoxyASTNode* f_ast_create_var_decl(const char *name, FoxyASTNode *initializer) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_VAR_DECL);
    if (!node) return NULL;
    node->as.var_decl_node.name = name ? strdup(name) : NULL;
    node->as.var_decl_node.initializer = initializer;
    return node;
}

FoxyASTNode* f_ast_create_assign(FoxyASTNode *left, FoxyASTNode *right, FoxyVM *vm) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ASSIGN);
    if (!node) return NULL;
    
    if (left && left->type == FOXY_AST_NODE_IDENTIFIER) {
        node->as.assign_node.name = left->as.identifier_node.name ? strdup(left->as.identifier_node.name) : NULL;
        f_ast_node_free(left, vm);
    } else {
        node->as.assign_node.name = NULL;
    }

    node->as.assign_node.value = right;
    return node;
}

FoxyASTNode* f_ast_create_if(FoxyASTNode *condition, FoxyASTNode *then_branch, FoxyASTNode *else_branch) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_IF);
    if (!node) return NULL;
    node->as.if_node.condition = condition;
    node->as.if_node.then_branch = then_branch;
    node->as.if_node.else_branch = else_branch;
    return node;
}

FoxyASTNode* f_ast_create_while(FoxyASTNode *condition, FoxyASTNode *body) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_WHILE);
    if (!node) return NULL;
    node->as.while_node.condition = condition;
    node->as.while_node.body = body;
    return node;
}

FoxyASTNode* f_ast_create_function(const char *name, FoxyASTNode *body) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_FUNCTION);
    if (!node) return NULL;
    node->as.function_node.name = name ? strdup(name) : NULL;
    node->as.function_node.param_names = NULL;
    node->as.function_node.param_count = 0;
    node->as.function_node.body = body;
    return node;
}

FoxyASTNode* f_ast_create_for(FoxyASTNode *init, FoxyASTNode *condition, FoxyASTNode *increment, FoxyASTNode *body) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_FOR);
    if (!node) return NULL;
    node->as.for_node.init = init;
    node->as.for_node.condition = condition;
    node->as.for_node.increment = increment;
    node->as.for_node.body = body;
    return node;
}

FoxyASTNode* f_ast_create_return(FoxyASTNode *value) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_RETURN);
    if (!node) return NULL;
    node->as.return_node.value = value;
    return node;
}

FoxyASTNode* f_ast_create_expr_stmt(FoxyASTNode *expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_EXPR_STMT);
    if (!node) return NULL;
    node->as.expr_stmt_node.expression = expr;
    return node;
}

FoxyASTNode* f_ast_create_env(void) {
    return f_ast_node_new(FOXY_AST_NODE_ENV);
}

FoxyASTNode* f_ast_create_env_create(FoxyASTNode *name_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ENV_CREATE);
    if (!node) return NULL;
    node->as.env_create_node.name_expr = name_expr;
    return node;
}

FoxyASTNode* f_ast_create_env_bind(FoxyASTNode *proc_expr, FoxyASTNode *env_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ENV_BIND);
    if (!node) return NULL;
    node->as.env_bind_node.process_expr = proc_expr;
    node->as.env_bind_node.env_expr = env_expr;
    return node;
}

FoxyASTNode* f_ast_create_popen(FoxyASTNode *callback_expr, FoxyASTNode *name_expr, FoxyASTNode *env_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_POPEN);
    if (!node) return NULL;
    node->as.popen_node.callback_expr = callback_expr;
    node->as.popen_node.name_expr = name_expr;
    node->as.popen_node.env_expr = env_expr;
    return node;
}