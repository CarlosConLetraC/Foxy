#include "f_ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FoxyASTNode* f_ast_node_new(FoxyASTNodeType type) {
    FoxyASTNode *node = malloc(sizeof(FoxyASTNode));
    if (!node) {
        fprintf(stderr, "[Foxy AST Error] Out of memory allocating AST node\n");
        exit(1);
    }
    memset(node, 0, sizeof(FoxyASTNode));
    node->type = type;
    return node;
}

/**
 * Crea un nodo AST para inicializar la semilla de un entorno (FOXCODE_ENV).
 */
FoxyASTNode* f_ast_create_env(void) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ENV);
    if (!node) return NULL;
    return node;
}

/**
 * Crea un nodo AST para instanciar e insertar un SharedEnv/Protocolo en el runtime.
 * 
 * @param name_expr Expresión que evalúa al nombre del protocolo (cadena).
 */
FoxyASTNode* f_ast_create_env_create(FoxyASTNode *name_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ENV_CREATE);
    if (!node) return NULL;

    node->as.env_create_node.name_expr = name_expr;
    return node;
}

/**
 * Crea un nodo AST para vincular un SharedEnv a un proceso en ejecución.
 * 
 * @param proc_expr Expresión del objeto Proceso objetivo.
 * @param env_expr Expresión del objeto SharedEnv/Protocolo.
 */
FoxyASTNode* f_ast_create_env_bind(FoxyASTNode *proc_expr, FoxyASTNode *env_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ENV_BIND);
    if (!node) return NULL;

    node->as.env_bind_node.process_expr = proc_expr;
    node->as.env_bind_node.env_expr     = env_expr;
    return node;
}

/**
 * Crea un nodo AST para la instanciación y arranque de un subproceso asíncrono (popen).
 * 
 * @param callback_expr Expresión ejecutable o función callback que correrá en el hilo.
 * @param name_expr Expresión opcional con el nombre identificador del subproceso (puede ser NULL).
 * @param env_expr Expresión opcional del SharedEnv/Protocolo asociado (puede ser NULL).
 */
FoxyASTNode* f_ast_create_popen(FoxyASTNode *callback_expr, FoxyASTNode *name_expr, FoxyASTNode *env_expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_POPEN);
    if (!node) return NULL;

    node->as.popen_node.callback_expr = callback_expr;
    node->as.popen_node.name_expr     = name_expr;
    node->as.popen_node.env_expr      = env_expr;
    return node;
}

void f_ast_node_free(FoxyASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case FOXY_AST_NODE_PROGRAM: {
            if (node->as.program_node.statements) {
                for (size_t i = 0; i < node->as.program_node.count; i++)
                    f_ast_node_free(node->as.program_node.statements[i]);
                free(node->as.program_node.statements);
            }
            break;
        }

        case FOXY_AST_NODE_INCLUDE:
            if (node->as.include_node.path)
                free(node->as.include_node.path);
            break;

        case FOXY_AST_NODE_CALL:
            if (node->as.call_node.callee_name)
                free(node->as.call_node.callee_name);
            if (node->as.call_node.arguments) {
                for (size_t i = 0; i < node->as.call_node.arg_count; i++)
                    f_ast_node_free(node->as.call_node.arguments[i]);
                free(node->as.call_node.arguments);
            }
            break;

        case FOXY_AST_NODE_IDENTIFIER:
            if (node->as.identifier_node.name)
                free(node->as.identifier_node.name);
            break;

        case FOXY_AST_NODE_BINARY_OP:
        case FOXY_AST_NODE_ASSIGN:
            f_ast_node_free(node->as.binary_node.left);
            f_ast_node_free(node->as.binary_node.right);
            break;

        case FOXY_AST_NODE_LITERAL:
            // Si FoxyValue contiene memoria dinámica (como strings) se libera aquí
            break;

        // =========================================================================
        // Integración de nodos de Concurrencia y SharedEnv
        // =========================================================================
        case FOXY_AST_NODE_ENV:
            // No contiene nodos hijos para liberar
            break;

        case FOXY_AST_NODE_ENV_CREATE:
            f_ast_node_free(node->as.env_create_node.name_expr);
            break;

        case FOXY_AST_NODE_ENV_BIND:
            f_ast_node_free(node->as.env_bind_node.process_expr);
            f_ast_node_free(node->as.env_bind_node.env_expr);
            break;

        case FOXY_AST_NODE_POPEN:
            f_ast_node_free(node->as.popen_node.callback_expr);
            if (node->as.popen_node.name_expr)
                f_ast_node_free(node->as.popen_node.name_expr);
            if (node->as.popen_node.env_expr)
                f_ast_node_free(node->as.popen_node.env_expr);
            break;

        default:
            break;
    }

    // Finalmente se libera el contenedor del nodo
    free(node);
}

FoxyASTNode* f_ast_create_program(void) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_PROGRAM);
    node->as.program_node.capacity = 16;
    node->as.program_node.statements = malloc(sizeof(FoxyASTNode*) * node->as.program_node.capacity);
    return node;
}

void f_ast_program_add(FoxyASTNode *program, FoxyASTNode *stmt) {
    if (!program || !stmt || program->type != FOXY_AST_NODE_PROGRAM) return;

    if (program->as.program_node.count >= program->as.program_node.capacity) {
        program->as.program_node.capacity *= 2;
        program->as.program_node.statements = realloc(
            program->as.program_node.statements, 
            sizeof(FoxyASTNode*) * program->as.program_node.capacity
        );
    }
    program->as.program_node.statements[program->as.program_node.count++] = stmt;
}

FoxyASTNode* f_ast_create_include(const char *path) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_INCLUDE);
    node->as.include_node.path = strdup(path);
    return node;
}

FoxyASTNode* f_ast_create_call(const char *callee) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_CALL);
    node->as.call_node.callee_name = strdup(callee);
    node->as.call_node.arg_capacity = 4;
    node->as.call_node.arguments = malloc(sizeof(FoxyASTNode*) * node->as.call_node.arg_capacity);
    return node;
}

FoxyASTNode* f_ast_create_identifier(const char *name) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
    if (!node) return NULL;
    node->as.identifier_node.name = strdup(name);
    return node;
}

FoxyASTNode* f_ast_create_binary_op(int op_token, FoxyASTNode *left, FoxyASTNode *right) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_BINARY_OP);
    if (!node) return NULL;
    node->as.binary_node.left = left;
    node->as.binary_node.right = right;
    node->as.binary_node.op_token = op_token;
    return node;
}

FoxyASTNode* f_ast_create_assign(FoxyASTNode *left, FoxyASTNode *right) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_ASSIGN);
    if (!node) return NULL;
    node->as.binary_node.left = left;
    node->as.binary_node.right = right;
    node->as.binary_node.op_token = '=';
    return node;
}

void f_ast_call_add_arg(FoxyASTNode *call_node, FoxyASTNode *arg) {
    if (!call_node || !arg || call_node->type != FOXY_AST_NODE_CALL) return;

    if (call_node->as.call_node.arg_count >= call_node->as.call_node.arg_capacity) {
        call_node->as.call_node.arg_capacity *= 2;
        call_node->as.call_node.arguments = realloc(
            call_node->as.call_node.arguments, 
            sizeof(FoxyASTNode*) * call_node->as.call_node.arg_capacity
        );
    }
    call_node->as.call_node.arguments[call_node->as.call_node.arg_count++] = arg;
}

FoxyASTNode* f_ast_create_expr_stmt(FoxyASTNode *expr) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_EXPR_STMT);
    if (!node) return NULL;
    node->as.expr_stmt_node.expression = expr;
    return node;
}

FoxyASTNode* f_ast_create_literal(FoxyValue val) {
    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_LITERAL);
    node->as.literal_node.value = val;
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

const char* f_ast_node_type_to_string(FoxyASTNodeType type) {
    switch (type) {
        #define F(node_type, name_str) case node_type: return name_str;
        FOXY_AST_NODE_LIST(F)
        #undef F
        default: return "UNKNOWN_NODE";
    }
}