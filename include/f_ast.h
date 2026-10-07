#pragma once

#include "f_settings.h"
#include "f_lexer.h"
#include "f_value.h"
#include "f_foxmode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * ============================================================================
 * FOXY ABSTRACT SYNTAX TREE ARCHITECTURE (f_ast.h)
 * ============================================================================
 * Representación en memoria de nodos AST mediante Tagged Unions y arreglos
 * dinámicos de nodos hijos. Diseñado para un consumo eficiente por el Parser,
 * análisis semántico (`f_symtable`) y el Emisor de Bytecode (`f_codegen`).
 * ============================================================================
 */

/* Forward Declaration */
typedef struct FoxyAstNode FoxyAstNode;

/**
 * @brief Categorías y Tipos de Nodos AST soportados en Foxy-Lang.
 */
#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_AST_KIND_LIST(F) \
    /* --- NODO RAÍZ / PROGRAMA --- */ \
    F(FOXY_AST_PROGRAM) \
    /* --- EXPRESIONES (EXPR) --- */ \
    F(FOXY_AST_EXPR_LITERAL) \
    F(FOXY_AST_EXPR_IDENTIFIER) \
    F(FOXY_AST_EXPR_UNARY) \
    F(FOXY_AST_EXPR_BINARY) \
    F(FOXY_AST_EXPR_ASSIGN) \
    F(FOXY_AST_EXPR_CALL) \
    F(FOXY_AST_EXPR_GET_MEMBER) \
    F(FOXY_AST_EXPR_SET_MEMBER) \
    F(FOXY_AST_EXPR_GET_INDEX) \
    F(FOXY_AST_EXPR_SET_INDEX) \
    F(FOXY_AST_EXPR_ARRAY_LITERAL) \
    F(FOXY_AST_EXPR_DICT_LITERAL) \
    F(FOXY_AST_EXPR_DICT_ENTRY) \
    /* --- SENTENCIAS / DECLARACIONES (STMT) --- */ \
    F(FOXY_AST_STMT_INCLUDE) \
    F(FOXY_AST_STMT_EXPR) \
    F(FOXY_AST_STMT_BLOCK) \
    F(FOXY_AST_STMT_VAR_DECL) \
    F(FOXY_AST_STMT_FUNC_DECL) \
    F(FOXY_AST_STMT_IF) \
    F(FOXY_AST_STMT_WHILE) \
    F(FOXY_AST_STMT_FOR) \
    F(FOXY_AST_STMT_FOREACH) \
    F(FOXY_AST_STMT_SWITCH) \
    F(FOXY_AST_STMT_CASE) \
    F(FOXY_AST_STMT_RETURN) \
    F(FOXY_AST_STMT_BREAK) \
    F(FOXY_AST_STMT_CONTINUE) \
    F(FOXY_AST_STMT_GOTO) \
    F(FOXY_AST_STMT_LABEL) \
    F(FOXY_AST_STMT_TRY) \
    F(FOXY_AST_STMT_CATCH)

#define F(kind) kind,
typedef enum FOXY_PACKED {
    FOXY_AST_KIND_LIST(F)
} FoxyAstKind;
#undef F
#else
typedef enum FOXY_PACKED {
    FOXY_AST_PROGRAM = 0,
    FOXY_AST_EXPR_LITERAL,
    FOXY_AST_EXPR_IDENTIFIER,
    FOXY_AST_EXPR_UNARY,
    FOXY_AST_EXPR_BINARY,
    FOXY_AST_EXPR_ASSIGN,
    FOXY_AST_EXPR_CALL,
    FOXY_AST_EXPR_GET_MEMBER,
    FOXY_AST_EXPR_SET_MEMBER,
    FOXY_AST_EXPR_GET_INDEX,
    FOXY_AST_EXPR_SET_INDEX,
    FOXY_AST_EXPR_ARRAY_LITERAL,
    FOXY_AST_EXPR_DICT_LITERAL,
    FOXY_AST_EXPR_DICT_ENTRY,
    FOXY_AST_STMT_INCLUDE,
    FOXY_AST_STMT_EXPR,
    FOXY_AST_STMT_BLOCK,
    FOXY_AST_STMT_VAR_DECL,
    FOXY_AST_STMT_FUNC_DECL,
    FOXY_AST_STMT_IF,
    FOXY_AST_STMT_WHILE,
    FOXY_AST_STMT_FOR,
    FOXY_AST_STMT_FOREACH,
    FOXY_AST_STMT_SWITCH,
    FOXY_AST_STMT_CASE,
    FOXY_AST_STMT_RETURN,
    FOXY_AST_STMT_BREAK,
    FOXY_AST_STMT_CONTINUE,
    FOXY_AST_STMT_GOTO,
    FOXY_AST_STMT_LABEL,
    FOXY_AST_STMT_TRY,
    FOXY_AST_STMT_CATCH
} FoxyAstKind;
#endif

/** Macro auxiliar para verificar si un tipo de nodo pertenece a Expresiones */
#define FOXY_AST_IS_EXPR_KIND(kind) \
    ((kind) >= FOXY_AST_EXPR_LITERAL && (kind) <= FOXY_AST_EXPR_DICT_ENTRY)

/** Macro auxiliar para verificar si un tipo de nodo pertenece a Sentencias */
#define FOXY_AST_IS_STMT_KIND(kind) \
    ((kind) >= FOXY_AST_STMT_EXPR && (kind) <= FOXY_AST_STMT_CATCH)

/**
 * @brief Lista dinámica de nodos AST para almacenar bloques, argumentos o parámetros.
 */
typedef struct {
    FoxyAstNode **nodes;            /* 8 bytes */
    size_t count;                   /* 8 bytes */
    size_t capacity;                /* 8 bytes */
} FoxyAstNodeList;

/* ========================================================================= */
/* DATOS ESPECÍFICOS SEGÚN LA CATEGORÍA DEL NODO                             */
/* ========================================================================= */

typedef struct {
    FoxyValue value;                /* 32 bytes */
} FoxyAstLiteral;

typedef struct {
    FoxyToken name;                 /* Token del identificador */
} FoxyAstIdentifier;

typedef struct {
    FoxyAstNode *operand;           /* 8 bytes */
    FoxyToken op;                   /* FoxyToken */
    uint8_t is_postfix;             /* 1 byte */
    uint8_t _pad[7];                /* 7 bytes para alineación de 8 bytes */
} FoxyAstUnary;

typedef struct {
    FoxyAstNode *left;              /* 8 bytes */
    FoxyAstNode *right;             /* 8 bytes */
    FoxyToken op;                   /* FoxyToken */
} FoxyAstBinary;

typedef struct {
    FoxyAstNode *target;            /* 8 bytes */
    FoxyAstNode *value;             /* 8 bytes */
    FoxyToken op;                   /* FoxyToken */
    uint8_t is_grouped;             /* 1 byte */
    uint8_t _pad[7];                /* 7 bytes para alineación de 8 bytes */
} FoxyAstAssign;

typedef struct {
    FoxyAstNode *callee;            /* 8 bytes */
    FoxyAstNodeList args;           /* 24 bytes */
} FoxyAstCall;

typedef struct {
    FoxyAstNode *object;            /* 8 bytes */
    FoxyToken member;               /* FoxyToken */
} FoxyAstGetMember;

typedef struct {
    FoxyAstNode *object;            /* 8 bytes */
    FoxyAstNode *value;             /* 8 bytes */
    FoxyToken member;               /* FoxyToken */
} FoxyAstSetMember;

typedef struct {
    FoxyAstNode *target;            /* 8 bytes */
    FoxyAstNode *index;             /* 8 bytes */
} FoxyAstGetIndex;

typedef struct {
    FoxyAstNode *target;            /* 8 bytes */
    FoxyAstNode *index;             /* 8 bytes */
    FoxyAstNode *value;             /* 8 bytes */
} FoxyAstSetIndex;

typedef struct {
    FoxyAstNodeList elements;       /* 24 bytes */
} FoxyAstArrayLiteral;

typedef struct {
    FoxyAstNodeList entries;        /* 24 bytes */
} FoxyAstDictLiteral;

typedef struct {
    FoxyAstNode *value;             /* 8 bytes */
    FoxyToken key;                  /* FoxyToken */
} FoxyAstDictEntry;

/* Sentencias y Declaraciones */

typedef struct {
    FoxyAstNode *initializer;       /* 8 bytes */
    FoxyToken name;                 /* FoxyToken */
    FoxyTokenType type_token;       /* Tipo enum/packed */
    uint8_t _pad[7];                /* Padding para múltiplo de 8 */
} FoxyAstVarDecl;

typedef struct {
    FoxyAstNode *body;              /* 8 bytes */
    FoxyAstNodeList params;         /* 24 bytes */
    FoxyToken name;                 /* FoxyToken */
} FoxyAstFuncDecl;

typedef struct {
    FoxyAstNode *condition;         /* 8 bytes */
    FoxyAstNode *then_branch;       /* 8 bytes */
    FoxyAstNode *else_branch;       /* 8 bytes */
} FoxyAstIfStmt;

typedef struct {
    FoxyAstNode *condition;         /* 8 bytes */
    FoxyAstNode *body;              /* 8 bytes */
} FoxyAstWhileStmt;

typedef struct {
    FoxyAstNode *init;              /* 8 bytes */
    FoxyAstNode *condition;         /* 8 bytes */
    FoxyAstNode *increment;         /* 8 bytes */
    FoxyAstNode *body;              /* 8 bytes */
} FoxyAstForStmt;

typedef struct {
    FoxyAstNode *iterable;          /* 8 bytes */
    FoxyAstNode *body;              /* 8 bytes */
    FoxyToken iterator_var;         /* FoxyToken */
} FoxyAstForeachStmt;

typedef struct {
    FoxyAstNode *condition;         /* 8 bytes */
    FoxyAstNodeList cases;          /* 24 bytes */
} FoxyAstSwitchStmt;

typedef struct {
    FoxyAstNode *expr;              /* 8 bytes */
    FoxyAstNodeList stmts;          /* 24 bytes */
} FoxyAstCaseStmt;

typedef struct {
    FoxyAstNode *value;             /* 8 bytes */
} FoxyAstReturnStmt;

typedef struct {
    FoxyToken label;                /* FoxyToken */
} FoxyAstGotoStmt;

typedef struct {
    FoxyAstNode *try_block;         /* 8 bytes */
    FoxyAstNode *finally_block;     /* 8 bytes */
    FoxyAstNodeList catch_blocks;   /* 24 bytes */
} FoxyAstTryStmt;

typedef struct {
    FoxyAstNode *body;              /* 8 bytes */
    FoxyToken var_name;             /* FoxyToken */
} FoxyAstCatchStmt;

typedef struct {
    FoxyLexer lexer;
    FoxyToken current_token;
    FoxyToken previous_token;
    uint32_t nest_depth;            /* 4 bytes (control de anidamiento) */
    uint16_t error_count;           /* 2 bytes (total de errores detectados) */
    uint8_t had_error : 1;          /* 1 byte contenedor para bitfields */
    uint8_t panic_mode : 1;
    uint8_t _reserved : 8;
} FoxyAstParser;

typedef struct {
    FoxyToken path;                 /* Token de la cadena con la ruta o módulo */
} FoxyAstIncludeStmt;

/**
 * @brief Estructura Principal de Nodo AST (Tagged Union)
 */
struct FoxyAstNode {
    union {
        FoxyAstNodeList program;
        FoxyAstLiteral literal;
        FoxyAstIdentifier identifier;
        FoxyAstUnary unary;
        FoxyAstBinary binary;
        FoxyAstAssign assign;
        FoxyAstCall call;
        FoxyAstGetMember get_member;
        FoxyAstSetMember set_member;
        FoxyAstGetIndex get_index;
        FoxyAstSetIndex set_index;
        FoxyAstArrayLiteral array_literal;
        FoxyAstDictLiteral dict_literal;
        FoxyAstDictEntry dict_entry;
        FoxyAstNode *expr_stmt;
        FoxyAstVarDecl var_decl;
        FoxyAstFuncDecl func_decl;
        FoxyAstIfStmt if_stmt;
        FoxyAstWhileStmt while_stmt;
        FoxyAstForStmt for_stmt;
        FoxyAstForeachStmt foreach_stmt;
        FoxyAstSwitchStmt switch_stmt;
        FoxyAstCaseStmt case_stmt;
        FoxyAstReturnStmt return_stmt;
        FoxyAstGotoStmt goto_stmt;
        FoxyAstTryStmt try_stmt;
        FoxyAstCatchStmt catch_stmt;
        FoxyAstIncludeStmt include_stmt;
    } as;                           /* Alineado a 8 bytes primero */

    FoxySourcePos pos;              /* Posición en el fuente */
    FoxyAstKind kind;               /* 1 byte al final */
    uint8_t _pad[15];               /* 15 bytes de padding explícito para alcanzar múltiplo de 16 */
};

/* ========================================================================= */
/* MÁSCARAS BITWISE (O(1)) PARA LEXER, TOKENS Y CATEGORÍAS DE NODOS          */
/* ========================================================================= */

/**
 * @brief Especificadores de Tipos Reservados
 */
#define FOXY_MASK_TYPE_SPECIFIER_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_KW_BOOL)     | \
    FOXY_BIT(FOX_TOKEN_KW_CHAR)     | \
    FOXY_BIT(FOX_TOKEN_KW_UCHAR)    | \
    FOXY_BIT(FOX_TOKEN_KW_SHORT)    | \
    FOXY_BIT(FOX_TOKEN_KW_USHORT)   | \
    FOXY_BIT(FOX_TOKEN_KW_INT)      | \
    FOXY_BIT(FOX_TOKEN_KW_UINT)     | \
    FOXY_BIT(FOX_TOKEN_KW_LONG)     | \
    FOXY_BIT(FOX_TOKEN_KW_ULONG)    | \
    FOXY_BIT(FOX_TOKEN_KW_LLONG)    | \
    FOXY_BIT(FOX_TOKEN_KW_ULLONG)   | \
    FOXY_BIT(FOX_TOKEN_KW_FLOAT)    | \
    FOXY_BIT(FOX_TOKEN_KW_DOUBLE)   | \
    FOXY_BIT(FOX_TOKEN_KW_LDOUBLE)  | \
    FOXY_BIT(FOX_TOKEN_KW_OBJECT)   | \
    FOXY_BIT(FOX_TOKEN_KW_STRUCT)   | \
    FOXY_BIT(FOX_TOKEN_KW_CLASS)    | \
    FOXY_BIT(FOX_TOKEN_KW_ENUM)       \
)

/**
 * @brief Tokens de inicio de sentencias (usados para f_ast_synchronize)
 */
#define FOXY_MASK_STMT_START_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_KW_CLASS)    | \
    FOXY_BIT(FOX_TOKEN_KW_STRUCT)   | \
    FOXY_BIT(FOX_TOKEN_KW_FUNCTION) | \
    FOXY_BIT(FOX_TOKEN_KW_FOR)      | \
    FOXY_BIT(FOX_TOKEN_KW_FOREACH)  | \
    FOXY_BIT(FOX_TOKEN_KW_IF)       | \
    FOXY_BIT(FOX_TOKEN_KW_WHILE)    | \
    FOXY_BIT(FOX_TOKEN_KW_RETURN)   | \
    FOXY_BIT(FOX_TOKEN_KW_SWITCH)   | \
    FOXY_BIT(FOX_TOKEN_KW_TRY)        \
)

/**
 * @brief Operadores de Asignación
 */
#define FOXY_MASK_ASSIGNMENT_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_ASSIGN)         | \
    FOXY_BIT(FOX_TOKEN_PLUS_ASSIGN)    | \
    FOXY_BIT(FOX_TOKEN_MINUS_ASSIGN)   | \
    FOXY_BIT(FOX_TOKEN_STAR_ASSIGN)    | \
    FOXY_BIT(FOX_TOKEN_SLASH_ASSIGN)   | \
    FOXY_BIT(FOX_TOKEN_PERCENT_ASSIGN) | \
    FOXY_BIT(FOX_TOKEN_POWER_ASSIGN)   | \
    FOXY_BIT(FOX_TOKEN_AND_ASSIGN)     | \
    FOXY_BIT(FOX_TOKEN_OR_ASSIGN)      | \
    FOXY_BIT(FOX_TOKEN_XOR_ASSIGN)     | \
    FOXY_BIT(FOX_TOKEN_LSHIFT_ASSIGN)  | \
    FOXY_BIT(FOX_TOKEN_RSHIFT_ASSIGN)    \
)

/**
 * @brief Operadores Aritméticos
 */
#define FOXY_MASK_ARITHMETIC_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_PLUS)    | \
    FOXY_BIT(FOX_TOKEN_MINUS)   | \
    FOXY_BIT(FOX_TOKEN_STAR)    | \
    FOXY_BIT(FOX_TOKEN_SLASH)   | \
    FOXY_BIT(FOX_TOKEN_PERCENT) | \
    FOXY_BIT(FOX_TOKEN_POWER)     \
)

/**
 * @brief Operadores de Bits
 */
#define FOXY_MASK_BITWISE_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_AMPERSAND) | \
    FOXY_BIT(FOX_TOKEN_PIPE)      | \
    FOXY_BIT(FOX_TOKEN_CARET)     | \
    FOXY_BIT(FOX_TOKEN_TILDE)     | \
    FOXY_BIT(FOX_TOKEN_LSHIFT)    | \
    FOXY_BIT(FOX_TOKEN_RSHIFT)      \
)

/**
 * @brief Operadores Relacionales y de Igualdad
 */
#define FOXY_MASK_EQUALITY_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_EQ)  | \
    FOXY_BIT(FOX_TOKEN_NEQ)   \
)

#define FOXY_MASK_RELATIONAL_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_LT) | \
    FOXY_BIT(FOX_TOKEN_GT) | \
    FOXY_BIT(FOX_TOKEN_LE) | \
    FOXY_BIT(FOX_TOKEN_GE)   \
)

#define FOXY_MASK_COMPARISON_TOKENS ( \
    FOXY_MASK_EQUALITY_TOKENS | \
    FOXY_MASK_RELATIONAL_TOKENS \
)

/**
 * @brief Operadores Lógicos
 */
#define FOXY_MASK_LOGICAL_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_AND)  | \
    FOXY_BIT(FOX_TOKEN_OR)   | \
    FOXY_BIT(FOX_TOKEN_BANG)   \
)

/**
 * @brief Operadores Unarios
 */
#define FOXY_MASK_UNARY_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_PLUS)      | \
    FOXY_BIT(FOX_TOKEN_MINUS)     | \
    FOXY_BIT(FOX_TOKEN_BANG)      | \
    FOXY_BIT(FOX_TOKEN_TILDE)     | \
    FOXY_BIT(FOX_TOKEN_HASH)      | \
    FOXY_BIT(FOX_TOKEN_AMPERSAND) | \
    FOXY_BIT(FOX_TOKEN_INC)       | \
    FOXY_BIT(FOX_TOKEN_DEC)         \
)

/**
 * @brief Literales Flotantes y Reales
 */
#define FOXY_MASK_FLOAT_LITERAL_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_FLOAT_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_DOUBLE_LITERAL)  | \
    FOXY_BIT(FOX_TOKEN_LDOUBLE_LITERAL)   \
)

/**
 * @brief Literales Numéricos Generales
 */
#define FOXY_MASK_NUMERIC_LITERAL_TOKENS ( \
    FOXY_BIT(FOX_TOKEN_INT_LITERAL)     | \
    FOXY_BIT(FOX_TOKEN_UINT_LITERAL)    | \
    FOXY_BIT(FOX_TOKEN_LONG_LITERAL)    | \
    FOXY_BIT(FOX_TOKEN_ULONG_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_LLONG_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_ULLONG_LITERAL)  | \
    FOXY_MASK_FLOAT_LITERAL_TOKENS      | \
    FOXY_BIT(FOX_TOKEN_NUMBER_LITERAL)    \
)

/**
 * @brief Todos los Literales Válidos
 */
#define FOXY_MASK_LITERAL_TOKENS ( \
    FOXY_MASK_NUMERIC_LITERAL_TOKENS   | \
    FOXY_BIT(FOX_TOKEN_CHAR_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_STRING_LITERAL) | \
    FOXY_BIT(FOX_TOKEN_KW_TRUE)        | \
    FOXY_BIT(FOX_TOKEN_KW_FALSE)       | \
    FOXY_BIT(FOX_TOKEN_KW_NULL)          \
)

/**
 * @brief Nodos del AST que contienen una única lista de sub-nodos (NodeList)
 */
#define FOXY_AST_MASK_SINGLE_LIST_NODES ( \
    FOXY_BIT(FOXY_AST_PROGRAM)            | \
    FOXY_BIT(FOXY_AST_STMT_BLOCK)         | \
    FOXY_BIT(FOXY_AST_EXPR_ARRAY_LITERAL) | \
    FOXY_BIT(FOXY_AST_EXPR_DICT_LITERAL)    \
)

/**
 * @brief Nodos Hoja del AST sin punteros dinámicos secundarios
 */
#define FOXY_AST_MASK_LEAF_NODES ( \
    FOXY_BIT(FOXY_AST_EXPR_IDENTIFIER)  | \
    FOXY_BIT(FOXY_AST_STMT_BREAK)       | \
    FOXY_BIT(FOXY_AST_STMT_CONTINUE)    | \
    FOXY_BIT(FOXY_AST_STMT_GOTO)        | \
    FOXY_BIT(FOXY_AST_STMT_LABEL)         \
)

/**
 * @brief Operadores Binarios / Infijos
 */
#define FOXY_MASK_BINARY_TOKENS ( \
    FOXY_MASK_ARITHMETIC_TOKENS | \
    FOXY_MASK_BITWISE_TOKENS    | \
    FOXY_MASK_COMPARISON_TOKENS | \
    FOXY_BIT(FOX_TOKEN_AND)     | \
    FOXY_BIT(FOX_TOKEN_OR)        \
)

/* ========================================================================= */
/* INLINES DE EVALUACIÓN EN O(1)                                             */
/* ========================================================================= */

static inline bool f_ast_is_type_specifier(uint32_t token_type) {
    return token_type < 64 && ((FOXY_MASK_TYPE_SPECIFIER_TOKENS & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_is_stmt_start(uint32_t token_type) {
    return token_type < 64 && ((FOXY_MASK_STMT_START_TOKENS & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_is_assignment_op(uint32_t token_type) {
    return token_type < 64 && ((FOXY_MASK_ASSIGNMENT_TOKENS & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_is_binary_op(uint32_t token_type) {
    uint64_t mask = FOXY_MASK_ARITHMETIC_TOKENS | 
                    FOXY_MASK_BITWISE_TOKENS    | 
                    FOXY_MASK_COMPARISON_TOKENS | 
                    FOXY_BIT(FOX_TOKEN_AND)     | 
                    FOXY_BIT(FOX_TOKEN_OR);
    return token_type < 64 && ((mask & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_is_unary_op(uint32_t token_type) {
    return token_type < 64 && ((FOXY_MASK_UNARY_TOKENS & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_is_literal(uint32_t token_type) {
    return token_type < 64 && ((FOXY_MASK_LITERAL_TOKENS & FOXY_BIT(token_type)) != 0);
}

static inline bool f_ast_kind_is_single_list(FoxyAstKind kind) {
    return (uint32_t)kind < 64 && ((FOXY_AST_MASK_SINGLE_LIST_NODES & FOXY_BIT(kind)) != 0);
}

static inline bool f_ast_kind_is_leaf(FoxyAstKind kind) {
    return (uint32_t)kind < 64 && ((FOXY_AST_MASK_LEAF_NODES & FOXY_BIT(kind)) != 0);
}

/* ========================================================================= */
/* API DE CONSTRUCCIÓN, GESTIÓN Y LIMPIEZA DE NODOS AST                      */
/* ========================================================================= */

FOXY_EXPORT FoxyAstNode *f_ast_create_node(FoxyAstKind kind, FoxySourcePos pos);
FOXY_EXPORT FoxyAstNode *f_ast_create_binary_node(FoxyToken op, FoxyAstNode *left, FoxyAstNode *right);
FOXY_EXPORT void f_ast_node_list_init(FoxyAstNodeList *list);
FOXY_EXPORT void f_ast_node_list_append(FoxyAstNodeList *list, FoxyAstNode *node);
FOXY_EXPORT void f_ast_append_child(FoxyAstNode *parent, FoxyAstNode *child);
FOXY_EXPORT void f_ast_node_list_free_shallow(FoxyAstNodeList *list);
FOXY_EXPORT void f_ast_free_node(FoxyAstNode *node);
FOXY_EXPORT void f_ast_print(const FoxyAstNode *node, int indent);
FOXY_EXPORT const char *f_ast_kind_to_string(FoxyAstKind kind);
FOXY_EXPORT char *f_ast_strdup(const char *src, size_t length);
FOXY_EXPORT void f_ast_parser_init(FoxyAstParser *parser, FILE *file, const char *filename);
FOXY_EXPORT FoxyAstNode *f_ast_parse_program(FoxyAstParser *parser);
FOXY_EXPORT FoxyValue f_ast_value_from_token(const FoxyToken *token);

#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_PRECEDENCE_LIST(F) \
    F(PREC_NONE) \
    F(PREC_ASSIGNMENT) /* = += -= */ \
    F(PREC_OR) /* || */ \
    F(PREC_AND) /* && */ \
    F(PREC_EQUALITY) /* == != */ \
    F(PREC_COMPARISON) /* < > <= >= */ \
    F(PREC_TERM) /* + - */ \
    F(PREC_FACTOR) /* * / % */ \
    F(PREC_UNARY) /* ! - ++ -- */ \
    F(PREC_CALL) /* . () [] */ \
    F(PREC_PRIMARY)

#define F(prec) prec,
typedef enum FOXY_PACKED {
    FOXY_PRECEDENCE_LIST(F)
} FoxyPrecedence;
#undef F
#else
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // = += -=
    PREC_OR,         // ||
    PREC_AND,        // &&
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * / %
    PREC_UNARY,      // ! - ++ --
    PREC_CALL,       // . () []
    PREC_PRIMARY
} FoxyPrecedence;
#endif

typedef FoxyAstNode *(*FoxyParseFn)(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);

typedef struct {
    FoxyParseFn prefix;        /* 8 bytes */
    FoxyParseFn infix;         /* 8 bytes */
    FoxyPrecedence precedence; /* 1 byte */
    uint8_t _pad[7];           /* 7 bytes explícitos para alineación a 8 bytes */
} FoxyParseRule;