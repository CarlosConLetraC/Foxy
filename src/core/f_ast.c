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

/* Forward Declarations internas */
static void f_ast_advance(FoxyAstParser *parser);
static void f_ast_synchronize(FoxyAstParser *parser);

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_expression(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence);
static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type);

/* Reglas de Parseo (deben coincidir con FoxyParseFn: parser, left, can_assign) */
static FoxyAstNode *f_ast_parse_literal(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_binary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_unary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_grouping(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_variable(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_super(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_self(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_assign(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_postfix(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_member(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_call(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_dict(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_array(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_index(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_ternary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);

static const FoxyParseRule rules[] = {
    [FOX_TOKEN_EOF]              = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_ERROR]            = { NULL,               NULL,              PREC_NONE },

    /* Identificadores y Literales */
    [FOX_TOKEN_IDENTIFIER]       = { f_ast_parse_variable,NULL,              PREC_NONE },
    [FOX_TOKEN_LABEL]            = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_INT_LITERAL]      = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_UINT_LITERAL]     = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LONG_LITERAL]     = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_ULONG_LITERAL]    = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LLONG_LITERAL]    = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_ULLONG_LITERAL]   = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_FLOAT_LITERAL]    = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_DOUBLE_LITERAL]   = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LDOUBLE_LITERAL]  = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_NUMBER_LITERAL]   = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_CHAR_LITERAL]     = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_STRING_LITERAL]   = { f_ast_parse_literal, NULL,              PREC_NONE },

    /* Palabras Clave / Especificadores / Tipos */
    [FOX_TOKEN_KW_GLOBAL]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_STATIC]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CONST]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_NULL]          = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_BOOL]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CHAR]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_UCHAR]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SHORT]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_USHORT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_INT]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_UINT]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_LONG]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ULONG]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_LLONG]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ULLONG]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FLOAT]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_DOUBLE]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_LDOUBLE]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_NUMBER]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_DICT]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_OBJECT]        = { NULL,               NULL,              PREC_NONE },

    /* Control de Flujo */
    [FOX_TOKEN_KW_IF]            = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ELSE]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ELSEIF]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_WHILE]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FOR]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FOREACH]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SWITCH]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CASE]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_DEFAULT]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_BREAK]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CONTINUE]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_RETURN]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_GOTO]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_TRY]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CATCH]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_EXCEPT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FINAL]         = { NULL,               NULL,              PREC_NONE },

    /* POO y Estructuras */
    [FOX_TOKEN_KW_CLASS]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_STRUCT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ENUM]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FROM]          = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FUNCTION]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_OVERRULE]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_INCLUDE]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_USE]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_EXPORT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SUPER]         = { f_ast_parse_super,  NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SELF]          = { f_ast_parse_self,   NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ANCESTOROF]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_DESCENDANTOF]  = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PARENTOF]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CHILDOF]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_TYPEOF]        = { f_ast_parse_unary,  NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_TRUE]          = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FALSE]         = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PRIVATE]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PROTECTED]     = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PUBLIC]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PRIVATE]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PROTECTED]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PUBLIC]       = { NULL,               NULL,              PREC_NONE },

    /* Métodos Marcados */
    [FOX_TOKEN_METHOD_NEW]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CAST]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_TOSTRING]  = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_ADD]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_SUB]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_MUL]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_DIV]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_POW]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_MOD]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CONCAT]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_UNM]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_NOT]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_EQ]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_NEQ]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_GT]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LE]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_GE]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BAND]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BOR]       = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BNOT]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BXOR]      = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LSHIFT]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_RSHIFT]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_FOREACH]   = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CLOSED]    = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LEN]       = { NULL,               NULL,              PREC_NONE },

    /* Operadores Aritméticos, Lógicos, Bitwise y Asignaciones */
    [FOX_TOKEN_PLUS]             = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_MINUS]            = { f_ast_parse_unary,  f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_STAR]             = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_SLASH]            = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_PERCENT]          = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_POWER]            = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_HASH]             = { f_ast_parse_unary,  NULL,              PREC_UNARY },
    [FOX_TOKEN_ASSIGN]           = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_PLUS_ASSIGN]      = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_MINUS_ASSIGN]     = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_STAR_ASSIGN]      = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_SLASH_ASSIGN]     = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_PERCENT_ASSIGN]   = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_POWER_ASSIGN]     = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_AND_ASSIGN]       = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_OR_ASSIGN]        = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_XOR_ASSIGN]       = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_LSHIFT_ASSIGN]    = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_RSHIFT_ASSIGN]    = { NULL,               f_ast_parse_assign, PREC_ASSIGNMENT },
    [FOX_TOKEN_INC]              = { f_ast_parse_unary,  f_ast_parse_postfix,PREC_CALL },
    [FOX_TOKEN_DEC]              = { f_ast_parse_unary,  f_ast_parse_postfix,PREC_CALL },
    [FOX_TOKEN_EQ]               = { NULL,               f_ast_parse_binary, PREC_EQUALITY },
    [FOX_TOKEN_NEQ]              = { NULL,               f_ast_parse_binary, PREC_EQUALITY },
    [FOX_TOKEN_LT]               = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_GT]               = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_LE]               = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_GE]               = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_BANG]             = { f_ast_parse_unary,  NULL,              PREC_UNARY },
    [FOX_TOKEN_AND]              = { NULL,               f_ast_parse_binary, PREC_AND },
    [FOX_TOKEN_OR]               = { NULL,               f_ast_parse_binary, PREC_OR },
    [FOX_TOKEN_AMPERSAND]        = { f_ast_parse_unary,  f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_PIPE]             = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_TILDE]            = { f_ast_parse_unary,  NULL,              PREC_UNARY },
    [FOX_TOKEN_CARET]            = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_LSHIFT]           = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_RSHIFT]           = { NULL,               f_ast_parse_binary, PREC_TERM },

    /* Delimitadores y Puntuación */
    [FOX_TOKEN_ARROW]            = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_FAT_ARROW]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_PTR_ARROW]        = { NULL,               f_ast_parse_member,PREC_CALL },
    [FOX_TOKEN_ELLIPSIS]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_LPAREN]           = { f_ast_parse_grouping,f_ast_parse_call,  PREC_CALL },
    [FOX_TOKEN_RPAREN]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_LBRACE]           = { f_ast_parse_dict,   NULL,              PREC_NONE },
    [FOX_TOKEN_RBRACE]           = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_LBRACKET]         = { f_ast_parse_array,  f_ast_parse_index, PREC_CALL },
    [FOX_TOKEN_RBRACKET]         = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_SEMICOLON]        = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_COLON]            = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_COMMA]            = { NULL,               NULL,              PREC_NONE },
    [FOX_TOKEN_DOT]              = { NULL,               f_ast_parse_member,PREC_CALL },
    [FOX_TOKEN_DOTDOT]           = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_QUESTION]         = { NULL,               f_ast_parse_ternary,PREC_ASSIGNMENT }
};

static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type) {
    return &rules[type];
}

static FoxyAstNode *f_ast_parse_unary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyToken op = parser->previous_token;
    FoxyAstNode *operand = f_ast_parse_precedence(parser, PREC_UNARY);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, op.pos);
    node->as.unary.op = op;
    node->as.unary.operand = operand;
    node->as.unary.is_postfix = 0;
    return node;
}

static FoxyAstNode *f_ast_parse_grouping(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *expr = f_ast_parse_expression(parser);
    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }
    return expr;
}

static FoxyAstNode *f_ast_parse_variable(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->previous_token.pos);
    node->as.identifier.name = parser->previous_token;
    return node;
}

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
        } else {
            // Si ocurrió un error y no entramos en panic_mode, avanzamos manualmente para evitar bucle infinito
            if (parser->had_error && !parser->panic_mode) {
                f_ast_advance(parser);
            }
        }
    }

    if (parser->had_error) {
        f_ast_free_node(program_node);
        return NULL;
    }

    return program_node;
}

static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence) {
    f_ast_advance(parser);
    
    FoxyParseFn prefix_rule = f_ast_get_rule(parser->previous_token.type)->prefix;
    if (prefix_rule == NULL) {
        fprintf(stderr, "[Foxy Parser Error] Line %u: Expected expression.\n", parser->previous_token.pos.line);
        parser->had_error = true;
        parser->panic_mode = true;
        return NULL;
    }

    bool can_assign = (precedence <= PREC_ASSIGNMENT);
    FoxyAstNode *node = prefix_rule(parser, NULL, can_assign);

    while (precedence <= f_ast_get_rule(parser->current_token.type)->precedence) {
        f_ast_advance(parser);
        FoxyParseFn infix_rule = f_ast_get_rule(parser->previous_token.type)->infix;
        if (infix_rule != NULL) {
            node = infix_rule(parser, node, can_assign);
        }
    }

    return node;
}

static FoxyAstNode *f_ast_parse_expression(FoxyAstParser *parser) {
    return f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
}

static FoxyAstNode *f_ast_parse_literal(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    (void)left;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_LITERAL, parser->previous_token.pos);
    node->as.literal.value = f_ast_value_from_token(&parser->previous_token);
    return node;
}

static FoxyAstNode *f_ast_parse_binary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxyTokenType op_type = parser->previous_token.type;
    FoxySourcePos pos = parser->previous_token.pos;
    const FoxyParseRule *rule = f_ast_get_rule(op_type);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_BINARY, pos);
    node->as.binary.op = parser->previous_token;
    node->as.binary.left = left; // Usa el nodo acumulado en la pila de Pratt

    // Recursión con precedencia superior
    node->as.binary.right = f_ast_parse_precedence(parser, (FoxyPrecedence)(rule->precedence + 1));
    return node;
}

static FoxyAstNode *f_ast_parse_statement(FoxyAstParser *parser) {
    // Para sentencias de expresión simples (ej: 1 + 2 * 3;)
    FoxyAstNode *expr = f_ast_parse_expression(parser);
    if (!expr) return NULL;

    FoxyAstNode *stmt = f_ast_create_node(FOXY_AST_STMT_EXPR, expr->pos);
    stmt->as.expr_stmt = expr;

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }
    return stmt;
}

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser) {
    if (f_ast_is_type_specifier(parser->current_token.type)) {
        FoxySourcePos pos = parser->current_token.pos;
        FoxyTokenType type_token = parser->current_token.type;
        f_ast_advance(parser);

        // Validar que el siguiente token sea el identificador/nombre de la variable
        if (parser->current_token.type != FOX_TOKEN_IDENTIFIER) {
            fprintf(stderr, "[Foxy Parser Error] Line %u: Expected variable name after type specifier.\n", 
                    parser->current_token.pos.line);
            parser->had_error = true;
            parser->panic_mode = true;
            return NULL;
        }

        FoxyToken name = parser->current_token;
        f_ast_advance(parser);

        FoxyAstNode *initializer = NULL;

        // Parsear inicializador opcional (= expr)
        if (parser->current_token.type == FOX_TOKEN_ASSIGN) {
            f_ast_advance(parser); // Consumir '='
            initializer = f_ast_parse_expression(parser);
        }

        // Exigir ';' al final de la declaración
        if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
            f_ast_advance(parser);
        } else {
            fprintf(stderr, "[Foxy Parser Error] Line %u: Expected ';' after variable declaration.\n", 
                    parser->current_token.pos.line);
            parser->had_error = true;
        }

        // Construir y retornar el nodo AST de la declaración
        FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_VAR_DECL, pos);
        node->as.var_decl.type_token = type_token;
        node->as.var_decl.name = name;
        node->as.var_decl.initializer = initializer;

        return node;
    }

    return f_ast_parse_statement(parser);
}

/* ============================================================================
 * IMPLEMENTACIÓN DE REGLAS DE PARSEO DE PRATT FALTANTES
 * ============================================================================ */

static FoxyAstNode *f_ast_parse_super(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->previous_token.pos);
    node->as.identifier.name = parser->previous_token;
    return node;
}

static FoxyAstNode *f_ast_parse_self(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->previous_token.pos);
    node->as.identifier.name = parser->previous_token;
    return node;
}

static FoxyAstNode *f_ast_parse_assign(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    if (!can_assign) {
        fprintf(stderr, "[Foxy Parser Error] Line %u: Invalid assignment target.\n", parser->previous_token.pos.line);
        parser->had_error = true;
    }

    FoxyToken op = parser->previous_token;
    FoxyAstNode *value = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_ASSIGN, op.pos);
    node->as.assign.op = op;
    node->as.assign.target = left;
    node->as.assign.value = value;
    node->as.assign.is_grouped = 0;
    return node;
}

static FoxyAstNode *f_ast_parse_postfix(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, parser->previous_token.pos);
    node->as.unary.op = parser->previous_token;
    node->as.unary.operand = left;
    node->as.unary.is_postfix = 1;
    return node;
}

static FoxyAstNode *f_ast_parse_member(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    FoxyToken member = parser->current_token;
    f_ast_advance(parser);

    if (can_assign && f_ast_is_assignment_op(parser->current_token.type)) {
        FoxyToken op = parser->current_token;
        f_ast_advance(parser);
        FoxyAstNode *value = f_ast_parse_expression(parser);

        FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_SET_MEMBER, op.pos);
        node->as.set_member.object = left;
        node->as.set_member.member = member;
        node->as.set_member.value = value;
        return node;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_GET_MEMBER, member.pos);
    node->as.get_member.object = left;
    node->as.get_member.member = member;
    return node;
}

static FoxyAstNode *f_ast_parse_call(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_CALL, parser->previous_token.pos);
    node->as.call.callee = left;
    f_ast_node_list_init(&node->as.call.args);

    if (parser->current_token.type != FOX_TOKEN_RPAREN) {
        do {
            f_ast_node_list_append(&node->as.call.args, f_ast_parse_expression(parser));
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true));
    }

    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_dict(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_DICT_LITERAL, parser->previous_token.pos);
    f_ast_node_list_init(&node->as.dict_literal.entries);

    while (parser->current_token.type != FOX_TOKEN_RBRACE && parser->current_token.type != FOX_TOKEN_EOF) {
        FoxyToken key = parser->current_token;
        f_ast_advance(parser);

        if (parser->current_token.type == FOX_TOKEN_COLON) {
            f_ast_advance(parser);
        } else {
            parser->had_error = true;
        }

        FoxyAstNode *val = f_ast_parse_expression(parser);
        FoxyAstNode *entry = f_ast_create_node(FOXY_AST_EXPR_DICT_ENTRY, key.pos);
        entry->as.dict_entry.key = key;
        entry->as.dict_entry.value = val;

        f_ast_node_list_append(&node->as.dict_literal.entries, entry);

        if (parser->current_token.type == FOX_TOKEN_COMMA) {
            f_ast_advance(parser);
        } else {
            break;
        }
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_array(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_ARRAY_LITERAL, parser->previous_token.pos);
    f_ast_node_list_init(&node->as.array_literal.elements);

    if (parser->current_token.type != FOX_TOKEN_RBRACKET) {
        do {
            f_ast_node_list_append(&node->as.array_literal.elements, f_ast_parse_expression(parser));
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true));
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_index(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    FoxyAstNode *index_expr = f_ast_parse_expression(parser);

    if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }

    if (can_assign && f_ast_is_assignment_op(parser->current_token.type)) {
        FoxyToken op = parser->current_token;
        f_ast_advance(parser);
        FoxyAstNode *val = f_ast_parse_expression(parser);

        FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_SET_INDEX, op.pos);
        node->as.set_index.target = left;
        node->as.set_index.index = index_expr;
        node->as.set_index.value = val;
        return node;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_GET_INDEX, left->pos);
    node->as.get_index.target = left;
    node->as.get_index.index = index_expr;
    return node;
}

static FoxyAstNode *f_ast_parse_ternary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxyAstNode *then_branch = f_ast_parse_expression(parser);

    if (parser->current_token.type == FOX_TOKEN_COLON) {
        f_ast_advance(parser);
    } else {
        parser->had_error = true;
    }

    FoxyAstNode *else_branch = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    FoxyAstNode *if_node = f_ast_create_node(FOXY_AST_STMT_IF, left->pos);
    if_node->as.if_stmt.condition = left;
    if_node->as.if_stmt.then_branch = then_branch;
    if_node->as.if_stmt.else_branch = else_branch;
    return if_node;
}