#include "f_ast.h"
#include "f_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * NOMBRES DE TIPOS DE NODOS AST (TABLA DE STRINGS VÍA X-MACRO)
 * ============================================================================ */
#if FOXY_COMPILER_SUPPORTS_XMACROS
static const char *const FOXY_AST_KIND_NAMES[] = {
    #define F(kind) #kind,
    FOXY_AST_KIND_LIST(F)
    #undef F
};
#else
static const char *const FOXY_AST_KIND_NAMES[] = {
    "FOXY_AST_PROGRAM",
    "FOXY_AST_EXPR_LITERAL",
    "FOXY_AST_EXPR_IDENTIFIER",
    "FOXY_AST_EXPR_UNARY",
    "FOXY_AST_EXPR_BINARY",
    "FOXY_AST_EXPR_ASSIGN",
    "FOXY_AST_EXPR_CALL",
    "FOXY_AST_EXPR_GET_MEMBER",
    "FOXY_AST_EXPR_SET_MEMBER",
    "FOXY_AST_EXPR_GET_INDEX",
    "FOXY_AST_EXPR_SET_INDEX",
    "FOXY_AST_EXPR_ARRAY_LITERAL",
    "FOXY_AST_EXPR_DICT_LITERAL",
    "FOXY_AST_EXPR_DICT_ENTRY",
    "FOXY_AST_STMT_EXPR",
    "FOXY_AST_STMT_BLOCK",
    "FOXY_AST_STMT_VAR_DECL",
    "FOXY_AST_STMT_FUNC_DECL",
    "FOXY_AST_STMT_IF",
    "FOXY_AST_STMT_WHILE",
    "FOXY_AST_STMT_FOR",
    "FOXY_AST_STMT_FOREACH",
    "FOXY_AST_STMT_SWITCH",
    "FOXY_AST_STMT_CASE",
    "FOXY_AST_STMT_RETURN",
    "FOXY_AST_STMT_BREAK",
    "FOXY_AST_STMT_CONTINUE",
    "FOXY_AST_STMT_GOTO",
    "FOXY_AST_STMT_LABEL",
    "FOXY_AST_STMT_TRY",
    "FOXY_AST_STMT_CATCH"
};
#endif

/* Forward Declarations de funciones internas */
static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser);

const char *f_ast_kind_to_string(FoxyAstKind kind) {
    return FOXY_AST_KIND_NAMES[kind];
}

char *f_ast_strdup(const char *src, size_t length) {
    if (!src) return NULL;
    char *dest = (char *)malloc(length + 1);
    if (!dest) {
        fprintf(stderr, "[Foxy AST Error] Memory allocation failed in f_ast_strdup.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(dest, src, length);
    dest[length] = '\0';
    return dest;
}

/* ============================================================================
 * CONSTRUCCIÓN Y GESTIÓN DE NODOS Y LISTAS AST
 * ============================================================================ */

FoxyAstNode *f_ast_create_node(FoxyAstKind kind, FoxySourcePos pos) {
    FoxyAstNode *node = (FoxyAstNode *)calloc(1, sizeof(FoxyAstNode));
    if (!node) {
        fprintf(stderr, "[Foxy AST Error] Out of memory creating AST node.\n");
        exit(EXIT_FAILURE);
    }
    node->kind = kind;
    node->pos = pos;
    return node;
}

void f_ast_node_list_init(FoxyAstNodeList *list) {
    if (!list) return;
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
}

void f_ast_node_list_append(FoxyAstNodeList *list, FoxyAstNode *node) {
    if (!list || !node) return;
    if (list->count + 1 > list->capacity) {
        size_t old_cap = list->capacity;
        list->capacity = (old_cap < FOXY_AST_CHILDREN_INITIAL_CAPACITY) 
            ? FOXY_AST_CHILDREN_INITIAL_CAPACITY 
            : old_cap * 2;
        
        FoxyAstNode **new_nodes = (FoxyAstNode **)realloc(list->nodes, sizeof(FoxyAstNode *) * list->capacity);
        if (!new_nodes) {
            fprintf(stderr, "[Foxy AST Error] Out of memory expanding AST NodeList.\n");
            exit(EXIT_FAILURE);
        }
        list->nodes = new_nodes;
    }
    list->nodes[list->count++] = node;
}

void f_ast_node_list_free_shallow(FoxyAstNodeList *list) {
    if (!list) return;
    if (list->nodes) {
        free(list->nodes);
        list->nodes = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

static void f_ast_free_node_list(FoxyAstNodeList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        f_ast_free_node(list->nodes[i]);
    }
    f_ast_node_list_free_shallow(list);
}

void f_ast_append_child(FoxyAstNode *parent, FoxyAstNode *child) {
    if (!parent || !child) return;
    if (parent->kind == FOXY_AST_PROGRAM || parent->kind == FOXY_AST_STMT_BLOCK) {
        f_ast_node_list_append(&parent->as.program, child);
    }
}

/* ============================================================================
 * LIBERACIÓN DE MEMORIA RECURSIVA (f_ast_free_node)
 * ============================================================================ */

void f_ast_free_node(FoxyAstNode *node) {
    if (!node) return;

    /* Fast path para nodos hoja sin memoria dinámica asignada */
    if (f_ast_kind_is_leaf(node->kind)) {
        free(node);
        return;
    }

    /* Fast path para contenedores de lista única (PROGRAM, BLOCK, ARRAY, DICT) */
    if (f_ast_kind_is_single_list(node->kind)) {
        f_ast_free_node_list(&node->as.program);
        free(node);
        return;
    }

#if USE_COMPUTED_GOTO
    #define F(kind) [kind] = &&L_FREE_##kind,
    static const void *dispatch_table[] = {
        FOXY_AST_KIND_LIST(F)
    };
    #undef F

    if ((size_t)node->kind < sizeof(dispatch_table) / sizeof(dispatch_table[0])) {
        goto *dispatch_table[node->kind];
    }
    goto L_FREE_END;

L_FREE_FOXY_AST_PROGRAM:
L_FREE_FOXY_AST_STMT_BLOCK:
L_FREE_FOXY_AST_EXPR_ARRAY_LITERAL:
L_FREE_FOXY_AST_EXPR_DICT_LITERAL:
L_FREE_FOXY_AST_EXPR_IDENTIFIER:
L_FREE_FOXY_AST_STMT_BREAK:
L_FREE_FOXY_AST_STMT_CONTINUE:
L_FREE_FOXY_AST_STMT_GOTO:
L_FREE_FOXY_AST_STMT_LABEL:
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_LITERAL:
    f_value_free(&node->as.literal.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_UNARY:
    f_ast_free_node(node->as.unary.operand);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_BINARY:
    f_ast_free_node(node->as.binary.left);
    f_ast_free_node(node->as.binary.right);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_ASSIGN:
    f_ast_free_node(node->as.assign.target);
    f_ast_free_node(node->as.assign.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_CALL:
    f_ast_free_node(node->as.call.callee);
    f_ast_free_node_list(&node->as.call.args);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_GET_MEMBER:
    f_ast_free_node(node->as.get_member.object);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_SET_MEMBER:
    f_ast_free_node(node->as.set_member.object);
    f_ast_free_node(node->as.set_member.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_GET_INDEX:
    f_ast_free_node(node->as.get_index.target);
    f_ast_free_node(node->as.get_index.index);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_SET_INDEX:
    f_ast_free_node(node->as.set_index.target);
    f_ast_free_node(node->as.set_index.index);
    f_ast_free_node(node->as.set_index.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_EXPR_DICT_ENTRY:
    f_ast_free_node(node->as.dict_entry.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_EXPR:
    f_ast_free_node(node->as.expr_stmt);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_VAR_DECL:
    f_ast_free_node(node->as.var_decl.initializer);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_FUNC_DECL:
    f_ast_free_node_list(&node->as.func_decl.params);
    f_ast_free_node(node->as.func_decl.body);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_IF:
    f_ast_free_node(node->as.if_stmt.condition);
    f_ast_free_node(node->as.if_stmt.then_branch);
    f_ast_free_node(node->as.if_stmt.else_branch);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_WHILE:
    f_ast_free_node(node->as.while_stmt.condition);
    f_ast_free_node(node->as.while_stmt.body);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_FOR:
    f_ast_free_node(node->as.for_stmt.init);
    f_ast_free_node(node->as.for_stmt.condition);
    f_ast_free_node(node->as.for_stmt.increment);
    f_ast_free_node(node->as.for_stmt.body);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_FOREACH:
    f_ast_free_node(node->as.foreach_stmt.iterable);
    f_ast_free_node(node->as.foreach_stmt.body);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_SWITCH:
    f_ast_free_node(node->as.switch_stmt.condition);
    f_ast_free_node_list(&node->as.switch_stmt.cases);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_CASE:
    f_ast_free_node(node->as.case_stmt.expr);
    f_ast_free_node_list(&node->as.case_stmt.stmts);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_RETURN:
    f_ast_free_node(node->as.return_stmt.value);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_TRY:
    f_ast_free_node(node->as.try_stmt.try_block);
    f_ast_free_node_list(&node->as.try_stmt.catch_blocks);
    f_ast_free_node(node->as.try_stmt.finally_block);
    goto L_FREE_END;

L_FREE_FOXY_AST_STMT_CATCH:
    f_ast_free_node(node->as.catch_stmt.body);
    goto L_FREE_END;

L_FREE_END:
#else
    switch (node->kind) {
        case FOXY_AST_EXPR_LITERAL:
            f_value_free(&node->as.literal.value);
            break;

        case FOXY_AST_EXPR_UNARY:
            f_ast_free_node(node->as.unary.operand);
            break;

        case FOXY_AST_EXPR_BINARY:
            f_ast_free_node(node->as.binary.left);
            f_ast_free_node(node->as.binary.right);
            break;

        case FOXY_AST_EXPR_ASSIGN:
            f_ast_free_node(node->as.assign.target);
            f_ast_free_node(node->as.assign.value);
            break;

        case FOXY_AST_EXPR_CALL:
            f_ast_free_node(node->as.call.callee);
            f_ast_free_node_list(&node->as.call.args);
            break;

        case FOXY_AST_EXPR_GET_MEMBER:
            f_ast_free_node(node->as.get_member.object);
            break;

        case FOXY_AST_EXPR_SET_MEMBER:
            f_ast_free_node(node->as.set_member.object);
            f_ast_free_node(node->as.set_member.value);
            break;

        case FOXY_AST_EXPR_GET_INDEX:
            f_ast_free_node(node->as.get_index.target);
            f_ast_free_node(node->as.get_index.index);
            break;

        case FOXY_AST_EXPR_SET_INDEX:
            f_ast_free_node(node->as.set_index.target);
            f_ast_free_node(node->as.set_index.index);
            f_ast_free_node(node->as.set_index.value);
            break;

        case FOXY_AST_EXPR_DICT_ENTRY:
            f_ast_free_node(node->as.dict_entry.value);
            break;

        case FOXY_AST_STMT_EXPR:
            f_ast_free_node(node->as.expr_stmt);
            break;

        case FOXY_AST_STMT_VAR_DECL:
            f_ast_free_node(node->as.var_decl.initializer);
            break;

        case FOXY_AST_STMT_FUNC_DECL:
            f_ast_free_node_list(&node->as.func_decl.params);
            f_ast_free_node(node->as.func_decl.body);
            break;

        case FOXY_AST_STMT_IF:
            f_ast_free_node(node->as.if_stmt.condition);
            f_ast_free_node(node->as.if_stmt.then_branch);
            f_ast_free_node(node->as.if_stmt.else_branch);
            break;

        case FOXY_AST_STMT_WHILE:
            f_ast_free_node(node->as.while_stmt.condition);
            f_ast_free_node(node->as.while_stmt.body);
            break;

        case FOXY_AST_STMT_FOR:
            f_ast_free_node(node->as.for_stmt.init);
            f_ast_free_node(node->as.for_stmt.condition);
            f_ast_free_node(node->as.for_stmt.increment);
            f_ast_free_node(node->as.for_stmt.body);
            break;

        case FOXY_AST_STMT_FOREACH:
            f_ast_free_node(node->as.foreach_stmt.iterable);
            f_ast_free_node(node->as.foreach_stmt.body);
            break;

        case FOXY_AST_STMT_SWITCH:
            f_ast_free_node(node->as.switch_stmt.condition);
            f_ast_free_node_list(&node->as.switch_stmt.cases);
            break;

        case FOXY_AST_STMT_CASE:
            f_ast_free_node(node->as.case_stmt.expr);
            f_ast_free_node_list(&node->as.case_stmt.stmts);
            break;

        case FOXY_AST_STMT_RETURN:
            f_ast_free_node(node->as.return_stmt.value);
            break;

        case FOXY_AST_STMT_TRY:
            f_ast_free_node(node->as.try_stmt.try_block);
            f_ast_free_node_list(&node->as.try_stmt.catch_blocks);
            f_ast_free_node(node->as.try_stmt.finally_block);
            break;

        case FOXY_AST_STMT_CATCH:
            f_ast_free_node(node->as.catch_stmt.body);
            break;

        default:
            break;
    }
#endif
    free(node);
}

/* ============================================================================
 * IMPRESIÓN Y DEPURACIÓN DEL AST (f_ast_print)
 * ============================================================================ */

static void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) printf("  ");
}

static void print_node_list(const FoxyAstNodeList *list, int indent) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        f_ast_print(list->nodes[i], indent);
    }
}

void f_ast_print(const FoxyAstNode *node, int indent) {
    if (!node) return;

    print_indent(indent);
    printf("%s (line %u, col %u)\n", 
           f_ast_kind_to_string(node->kind), 
           node->pos.line, 
           node->pos.column);

    if (f_ast_kind_is_single_list(node->kind)) {
        print_node_list(&node->as.program, indent + 1);
        return;
    }

    switch (node->kind) {
        case FOXY_AST_EXPR_LITERAL:
            print_indent(indent + 1);
            printf("Literal: ");
            f_value_print(node->as.literal.value);
            printf("\n");
            break;

        case FOXY_AST_EXPR_IDENTIFIER:
            print_indent(indent + 1);
            printf("Identifier: %.*s\n", node->as.identifier.name.length, node->as.identifier.name.start);
            break;

        case FOXY_AST_EXPR_UNARY:
            print_indent(indent + 1);
            printf("Op: %.*s (postfix: %s)\n", 
                   node->as.unary.op.length, node->as.unary.op.start,
                   node->as.unary.is_postfix ? "true" : "false");
            f_ast_print(node->as.unary.operand, indent + 1);
            break;

        case FOXY_AST_EXPR_BINARY:
            print_indent(indent + 1);
            printf("Op: %.*s\n", node->as.binary.op.length, node->as.binary.op.start);
            f_ast_print(node->as.binary.left, indent + 1);
            f_ast_print(node->as.binary.right, indent + 1);
            break;

        case FOXY_AST_EXPR_ASSIGN:
            print_indent(indent + 1);
            printf("Op: %.*s (grouped: %s)\n", 
                   node->as.assign.op.length, node->as.assign.op.start,
                   node->as.assign.is_grouped ? "true" : "false");
            f_ast_print(node->as.assign.target, indent + 1);
            f_ast_print(node->as.assign.value, indent + 1);
            break;

        case FOXY_AST_EXPR_CALL:
            f_ast_print(node->as.call.callee, indent + 1);
            print_node_list(&node->as.call.args, indent + 1);
            break;

        case FOXY_AST_EXPR_GET_MEMBER:
            print_indent(indent + 1);
            printf("Member: %.*s\n", node->as.get_member.member.length, node->as.get_member.member.start);
            f_ast_print(node->as.get_member.object, indent + 1);
            break;

        case FOXY_AST_EXPR_SET_MEMBER:
            print_indent(indent + 1);
            printf("Member: %.*s\n", node->as.set_member.member.length, node->as.set_member.member.start);
            f_ast_print(node->as.set_member.object, indent + 1);
            f_ast_print(node->as.set_member.value, indent + 1);
            break;

        case FOXY_AST_EXPR_GET_INDEX:
            f_ast_print(node->as.get_index.target, indent + 1);
            f_ast_print(node->as.get_index.index, indent + 1);
            break;

        case FOXY_AST_EXPR_SET_INDEX:
            f_ast_print(node->as.set_index.target, indent + 1);
            f_ast_print(node->as.set_index.index, indent + 1);
            f_ast_print(node->as.set_index.value, indent + 1);
            break;

        case FOXY_AST_EXPR_DICT_ENTRY:
            print_indent(indent + 1);
            printf("Key: %.*s\n", node->as.dict_entry.key.length, node->as.dict_entry.key.start);
            f_ast_print(node->as.dict_entry.value, indent + 1);
            break;

        case FOXY_AST_STMT_EXPR:
            f_ast_print(node->as.expr_stmt, indent + 1);
            break;

        case FOXY_AST_STMT_VAR_DECL:
            print_indent(indent + 1);
            printf("Var: %.*s (type_token: %d)\n", 
                   node->as.var_decl.name.length, node->as.var_decl.name.start, 
                   node->as.var_decl.type_token);
            if (node->as.var_decl.initializer) {
                f_ast_print(node->as.var_decl.initializer, indent + 1);
            }
            break;

        case FOXY_AST_STMT_FUNC_DECL:
            print_indent(indent + 1);
            printf("Function: %.*s\n", node->as.func_decl.name.length, node->as.func_decl.name.start);
            print_node_list(&node->as.func_decl.params, indent + 1);
            f_ast_print(node->as.func_decl.body, indent + 1);
            break;

        case FOXY_AST_STMT_IF:
            f_ast_print(node->as.if_stmt.condition, indent + 1);
            f_ast_print(node->as.if_stmt.then_branch, indent + 1);
            if (node->as.if_stmt.else_branch) {
                f_ast_print(node->as.if_stmt.else_branch, indent + 1);
            }
            break;

        case FOXY_AST_STMT_WHILE:
            f_ast_print(node->as.while_stmt.condition, indent + 1);
            f_ast_print(node->as.while_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_FOR:
            if (node->as.for_stmt.init) f_ast_print(node->as.for_stmt.init, indent + 1);
            if (node->as.for_stmt.condition) f_ast_print(node->as.for_stmt.condition, indent + 1);
            if (node->as.for_stmt.increment) f_ast_print(node->as.for_stmt.increment, indent + 1);
            f_ast_print(node->as.for_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_FOREACH:
            print_indent(indent + 1);
            printf("Iterator: %.*s\n", node->as.foreach_stmt.iterator_var.length, node->as.foreach_stmt.iterator_var.start);
            f_ast_print(node->as.foreach_stmt.iterable, indent + 1);
            f_ast_print(node->as.foreach_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_SWITCH:
            f_ast_print(node->as.switch_stmt.condition, indent + 1);
            print_node_list(&node->as.switch_stmt.cases, indent + 1);
            break;

        case FOXY_AST_STMT_CASE:
            if (node->as.case_stmt.expr) {
                f_ast_print(node->as.case_stmt.expr, indent + 1);
            } else {
                print_indent(indent + 1);
                printf("Default Case\n");
            }
            print_node_list(&node->as.case_stmt.stmts, indent + 1);
            break;

        case FOXY_AST_STMT_RETURN:
            if (node->as.return_stmt.value) {
                f_ast_print(node->as.return_stmt.value, indent + 1);
            }
            break;

        case FOXY_AST_STMT_GOTO:
            print_indent(indent + 1);
            printf("Goto Target: %.*s\n", node->as.goto_stmt.label.length, node->as.goto_stmt.label.start);
            break;

        case FOXY_AST_STMT_TRY:
            f_ast_print(node->as.try_stmt.try_block, indent + 1);
            print_node_list(&node->as.try_stmt.catch_blocks, indent + 1);
            if (node->as.try_stmt.finally_block) {
                f_ast_print(node->as.try_stmt.finally_block, indent + 1);
            }
            break;

        case FOXY_AST_STMT_CATCH:
            print_indent(indent + 1);
            printf("Catch Var: %.*s\n", node->as.catch_stmt.var_name.length, node->as.catch_stmt.var_name.start);
            f_ast_print(node->as.catch_stmt.body, indent + 1);
            break;

        default:
            break;
    }
}

/* ============================================================================
 * CONVERSIÓN DE TOKENS A VALORES
 * ============================================================================ */

FoxyValue f_ast_value_from_token(const FoxyToken *token) {
    if (!token) return f_value_new_null();

    if (f_ast_is_literal(token->type)) {
        if (token->type == FOX_TOKEN_KW_TRUE) return f_value_new_bool(true);
        if (token->type == FOX_TOKEN_KW_FALSE) return f_value_new_bool(false);
        if (token->type == FOX_TOKEN_KW_NULL) return f_value_new_null();

        if (token->type >= FOX_TOKEN_INT_LITERAL && token->type <= FOX_TOKEN_NUMBER_LITERAL) {
            long val = strtol(token->start, NULL, 10);
            return f_value_new_int((int)val);
        }

        if (token->type >= FOX_TOKEN_FLOAT_LITERAL && token->type <= FOX_TOKEN_LDOUBLE_LITERAL) {
            double val = strtod(token->start, NULL);
            return f_value_new_double(val);
        }
    }

    return f_value_new_null();
}

/* ============================================================================
 * UTILIDADES DE PARSER Y SINCRONIZACIÓN
 * ============================================================================ */

void f_ast_parser_init(FoxyAstParser *parser, FILE *file, const char *filename) {
    if (!parser) return;
    
    f_lexer_init_file(&parser->lexer, file, filename);
    parser->had_error = false;
    parser->panic_mode = false;
    parser->current_token = f_lexer_next_token(&parser->lexer);
}

static void f_ast_advance(FoxyAstParser *parser) {
    if (!parser) return;
    parser->previous_token = parser->current_token;

    for (;;) {
        parser->current_token = f_lexer_next_token(&parser->lexer);
        if (parser->current_token.type != FOX_TOKEN_ERROR) break;

        parser->had_error = true;
        fprintf(stderr, "[Foxy Parser Error] Line %u: %.*s\n", 
                parser->current_token.pos.line, 
                parser->current_token.length, 
                parser->current_token.start);
    }
}

static void f_ast_synchronize(FoxyAstParser *parser) {
    if (!parser) return;
    parser->panic_mode = false;

    while (parser->current_token.type != FOX_TOKEN_EOF) {
        if (parser->previous_token.type == FOX_TOKEN_SEMICOLON) return;

        /* Evaluación en O(1) usando inlines con máscaras de bits bitwise */
        if (f_ast_is_type_specifier(parser->current_token.type) ||
            f_ast_is_stmt_start(parser->current_token.type)) {
            return;
        }

        f_ast_advance(parser);
    }
}

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser) {
    f_ast_advance(parser);
    return NULL;
}

FoxyAstNode *f_ast_parse_program(FoxyAstParser *parser) {
    if (!parser) return NULL;

    FoxyAstNode *program_node = f_ast_create_node(FOXY_AST_PROGRAM, parser->current_token.pos);
    if (!program_node) {
        parser->had_error = true;
        return NULL;
    }

    while (parser->current_token.type != FOX_TOKEN_EOF) {
        if (parser->panic_mode) {
            f_ast_synchronize(parser);
            if (parser->current_token.type == FOX_TOKEN_EOF) break;
        }

        FoxyAstNode *stmt = f_ast_parse_declaration(parser);

        if (stmt) {
            f_ast_append_child(program_node, stmt);
        } else if (!parser->had_error) {
            f_ast_advance(parser);
        }
    }

    if (parser->had_error) {
        f_ast_free_node(program_node);
        return NULL;
    }

    return program_node;
}