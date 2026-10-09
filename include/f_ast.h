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
    F(FOXY_AST_STMT_CATCH) \
    F(FOXY_AST_STMT_LAMBDA)

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
    FOXY_AST_STMT_CATCH,
    FOXY_AST_STMT_LAMBDA
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
    FoxyAstNode *initializer;       /* 8 bytes: Expresión del valor asignado */
    FoxyAstNode *array_size;        /* 8 bytes: Expresión del tamaño (NULL si no se especificó) */
    FoxyToken name;                 /* 32 bytes: Token del identificador (VAR_NAME) */
    FoxyTokenType type_token;       /* 4 bytes: FOX_TOKEN_KW_* o FOX_TOKEN_EOF */
    uint16_t is_array  : 1;         /* Bitfields empacados en uint16_t (2 bytes) */
    uint16_t is_global : 1;
    uint16_t is_static : 1;
    uint16_t is_const  : 1;
    uint16_t is_hybrid : 1;
    uint16_t _pad_bits : 11;
    uint16_t _pad_align;            /* 2 bytes para completar exactamente 48 bytes (múltiplo de 8) */
} FoxyAstVarDecl;

typedef struct {
    FoxyAstNode *body;              /* 8 bytes */
    FoxyAstNodeList params;         /* 24 bytes */
    FoxyToken name;                 /* 32 bytes */
    uint32_t is_overrule : 1;       /* 4 bytes con 31 bits de padding implícito */
    uint32_t _pad_bits : 31;        /* Total = 68 -> alineado a 72 bytes por padding de estructura */
} FoxyAstFuncDecl;

typedef struct FoxyAstBlockStmt {
    FoxyAstNodeList statements;
} FoxyAstBlockStmt;

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

typedef struct {
    FoxyAstNodeList params;         /* Lista de nodos FOXY_AST_STMT_VAR_DECL o IDENTIFIER */
    FoxyAstNode *body;              /* Expresión o Bloque */
} FoxyAstLambdaExpr;

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
        FoxyAstBlockStmt block_stmt;
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
        FoxyAstLambdaExpr lambda_stmt;
    } as;                           /* Alineado a 8 bytes */

    FoxySourcePos pos;              /* 8 bytes: Posición en el fuente */
    FoxyAstKind kind;               /* 1 byte (packed enum) */
    uint8_t _pad[7];                /* 7 bytes de padding para completar el múltiplo de 8 exacto */
};

/* ========================================================================= */
/* MÁSCARAS BITWISE (O(1)) PARA LEXER, TOKENS Y CATEGORÍAS DE NODOS          */
/* ========================================================================= */

/* Macros helper para encadenar Bitwise OR sin romper la sintaxis del C preprocessor */
#define FOXY_X_BIT_0(t) FOXY_BIT_0(t) |
#define FOXY_X_BIT_1(t) FOXY_BIT_1(t) |

/**
 * @brief Lista X-Macro de Especificadores de Tipos Reservados
 */
#define FOXY_TYPE_SPECIFIER_LIST(F) \
    F(FOX_TOKEN_KW_BOOL)            \
    F(FOX_TOKEN_KW_CHAR)            \
    F(FOX_TOKEN_KW_UCHAR)           \
    F(FOX_TOKEN_KW_SHORT)           \
    F(FOX_TOKEN_KW_USHORT)          \
    F(FOX_TOKEN_KW_INT)             \
    F(FOX_TOKEN_KW_UINT)            \
    F(FOX_TOKEN_KW_LONG)            \
    F(FOX_TOKEN_KW_ULONG)           \
    F(FOX_TOKEN_KW_LLONG)           \
    F(FOX_TOKEN_KW_ULLONG)          \
    F(FOX_TOKEN_KW_FLOAT)           \
    F(FOX_TOKEN_KW_DOUBLE)          \
    F(FOX_TOKEN_KW_LDOUBLE)         \
    F(FOX_TOKEN_KW_NUMBER)          \
    F(FOX_TOKEN_KW_DICT)            \
    F(FOX_TOKEN_KW_OBJECT)          \
    F(FOX_TOKEN_KW_STRUCT)          \
    F(FOX_TOKEN_KW_CLASS)           \
    F(FOX_TOKEN_KW_ENUM)

/* Banco 0: Filtra y activa únicamente tokens con ID de 0 a 63 */
#define FOXY_MASK_TYPE_SPECIFIER_TOKENS_0 (FOXY_TYPE_SPECIFIER_LIST(FOXY_X_BIT_0) 0ULL)

/* Banco 1: Filtra y activa únicamente tokens con ID de 64 a 127 */
#define FOXY_MASK_TYPE_SPECIFIER_TOKENS_1 (FOXY_TYPE_SPECIFIER_LIST(FOXY_X_BIT_1) 0ULL)

/**
 * @brief Tokens de inicio de sentencias
 */
#define FOXY_MASK_STMT_START_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_KW_CLASS)      | \
    FOXY_BIT(FOX_TOKEN_KW_STRUCT)     | \
    FOXY_BIT(FOX_TOKEN_KW_ENUM)       | \
    FOXY_BIT(FOX_TOKEN_KW_FUNCTION)   | \
    FOXY_BIT(FOX_TOKEN_KW_FOR)        | \
    FOXY_BIT(FOX_TOKEN_KW_FOREACH)    | \
    FOXY_BIT(FOX_TOKEN_KW_IF)         | \
    FOXY_BIT(FOX_TOKEN_KW_WHILE)      | \
    FOXY_BIT(FOX_TOKEN_KW_RETURN)     | \
    FOXY_BIT(FOX_TOKEN_KW_SWITCH)     | \
    FOXY_BIT(FOX_TOKEN_KW_TRY)        | \
    FOXY_BIT(FOX_TOKEN_KW_INCLUDE)      \
)
#define FOXY_MASK_STMT_START_TOKENS_1 ((uint64_t)0)

/**
 * @brief Operadores de Asignación
 */
#define FOXY_MASK_ASSIGNMENT_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_ASSIGN)        | \
    FOXY_BIT(FOX_TOKEN_PLUS_ASSIGN)   | \
    FOXY_BIT(FOX_TOKEN_MINUS_ASSIGN)  | \
    FOXY_BIT(FOX_TOKEN_STAR_ASSIGN)   | \
    FOXY_BIT(FOX_TOKEN_SLASH_ASSIGN)  | \
    FOXY_BIT(FOX_TOKEN_PERCENT_ASSIGN)| \
    FOXY_BIT(FOX_TOKEN_POWER_ASSIGN)  | \
    FOXY_BIT(FOX_TOKEN_AND_ASSIGN)    | \
    FOXY_BIT(FOX_TOKEN_OR_ASSIGN)     | \
    FOXY_BIT(FOX_TOKEN_XOR_ASSIGN)    | \
    FOXY_BIT(FOX_TOKEN_LSHIFT_ASSIGN) | \
    FOXY_BIT(FOX_TOKEN_RSHIFT_ASSIGN)   \
)
#define FOXY_MASK_ASSIGNMENT_TOKENS_1 ((uint64_t)0)

/**
 * @brief Operadores Aritméticos
 */
#define FOXY_MASK_ARITHMETIC_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_PLUS)          | \
    FOXY_BIT(FOX_TOKEN_MINUS)         | \
    FOXY_BIT(FOX_TOKEN_STAR)          | \
    FOXY_BIT(FOX_TOKEN_SLASH)         | \
    FOXY_BIT(FOX_TOKEN_PERCENT)       | \
    FOXY_BIT(FOX_TOKEN_POWER)           \
)
#define FOXY_MASK_ARITHMETIC_TOKENS_1 ((uint64_t)0)

/**
 * @brief Operadores de Bits
 */
#define FOXY_MASK_BITWISE_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_AMPERSAND)  | \
    FOXY_BIT(FOX_TOKEN_PIPE)       | \
    FOXY_BIT(FOX_TOKEN_CARET)      | \
    FOXY_BIT(FOX_TOKEN_TILDE)      | \
    FOXY_BIT(FOX_TOKEN_LSHIFT)     | \
    FOXY_BIT(FOX_TOKEN_RSHIFT)       \
)
#define FOXY_MASK_BITWISE_TOKENS_1 ((uint64_t)0)

/**
 * @brief Operadores Relacionales y de Igualdad
 */
#define FOXY_MASK_EQUALITY_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_EQ)          | \
    FOXY_BIT(FOX_TOKEN_NEQ)           \
)
#define FOXY_MASK_EQUALITY_TOKENS_1 ((uint64_t)0)

#define FOXY_MASK_RELATIONAL_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_LT)            | \
    FOXY_BIT(FOX_TOKEN_GT)            | \
    FOXY_BIT(FOX_TOKEN_LE)            | \
    FOXY_BIT(FOX_TOKEN_GE)              \
)
#define FOXY_MASK_RELATIONAL_TOKENS_1 ((uint64_t)0)

#define FOXY_MASK_COMPARISON_TOKENS_0 ( \
    FOXY_MASK_EQUALITY_TOKENS_0       | \
    FOXY_MASK_RELATIONAL_TOKENS_0       \
)
#define FOXY_MASK_COMPARISON_TOKENS_1 ( \
    FOXY_MASK_EQUALITY_TOKENS_1       | \
    FOXY_MASK_RELATIONAL_TOKENS_1       \
)

/**
 * @brief Operadores Lógicos
 */
#define FOXY_MASK_LOGICAL_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_AND)        | \
    FOXY_BIT(FOX_TOKEN_OR)         | \
    FOXY_BIT(FOX_TOKEN_BANG)         \
)
#define FOXY_MASK_LOGICAL_TOKENS_1 ((uint64_t)0)

/**
 * @brief Operadores Unarios
 */
#define FOXY_MASK_UNARY_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_PLUS)     | \
    FOXY_BIT(FOX_TOKEN_MINUS)    | \
    FOXY_BIT(FOX_TOKEN_BANG)     | \
    FOXY_BIT(FOX_TOKEN_TILDE)    | \
    FOXY_BIT(FOX_TOKEN_HASH)     | \
    FOXY_BIT(FOX_TOKEN_AMPERSAND)| \
    FOXY_BIT(FOX_TOKEN_INC)      | \
    FOXY_BIT(FOX_TOKEN_DEC)        \
)
#define FOXY_MASK_UNARY_TOKENS_1 ((uint64_t)0)

/**
 * @brief Literales Flotantes y Reales
 */
#define FOXY_MASK_FLOAT_LITERAL_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_FLOAT_LITERAL)    | \
    FOXY_BIT(FOX_TOKEN_DOUBLE_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_LDOUBLE_LITERAL)    \
)
#define FOXY_MASK_FLOAT_LITERAL_TOKENS_1 ((uint64_t)0)

/**
 * @brief Literales Numéricos Generales
 */
#define FOXY_MASK_NUMERIC_LITERAL_TOKENS_0 ( \
    FOXY_BIT(FOX_TOKEN_INT_LITERAL)     | \
    FOXY_BIT(FOX_TOKEN_UINT_LITERAL)    | \
    FOXY_BIT(FOX_TOKEN_LONG_LITERAL)    | \
    FOXY_BIT(FOX_TOKEN_ULONG_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_LLONG_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_ULLONG_LITERAL)  | \
    FOXY_MASK_FLOAT_LITERAL_TOKENS_0    | \
    FOXY_BIT(FOX_TOKEN_NUMBER_LITERAL)    \
)
#define FOXY_MASK_NUMERIC_LITERAL_TOKENS_1 ( \
    FOXY_MASK_FLOAT_LITERAL_TOKENS_1 \
)

/**
 * @brief Todos los Literales Válidos
 */
#define FOXY_MASK_LITERAL_TOKENS_0 ( \
    FOXY_MASK_NUMERIC_LITERAL_TOKENS_0 | \
    FOXY_BIT(FOX_TOKEN_CHAR_LITERAL)   | \
    FOXY_BIT(FOX_TOKEN_STRING_LITERAL) | \
    FOXY_BIT(FOX_TOKEN_KW_TRUE)        | \
    FOXY_BIT(FOX_TOKEN_KW_FALSE)       | \
    FOXY_BIT(FOX_TOKEN_KW_NULL)          \
)
#define FOXY_MASK_LITERAL_TOKENS_1 ( \
    FOXY_MASK_NUMERIC_LITERAL_TOKENS_1 \
)

/**
 * @brief Operadores Binarios / Infijos
 */
#define FOXY_MASK_BINARY_TOKENS_0 ( \
    FOXY_MASK_ARITHMETIC_TOKENS_0 | \
    FOXY_MASK_BITWISE_TOKENS_0    | \
    FOXY_MASK_COMPARISON_TOKENS_0 | \
    FOXY_BIT(FOX_TOKEN_AND)       | \
    FOXY_BIT(FOX_TOKEN_OR)          \
)
#define FOXY_MASK_BINARY_TOKENS_1 ( \
    FOXY_MASK_ARITHMETIC_TOKENS_1 | \
    FOXY_MASK_BITWISE_TOKENS_1    | \
    FOXY_MASK_COMPARISON_TOKENS_1   \
)

/**
 * @brief Máscaras para Categorías AST (FoxyAstKind)
 */
#define FOXY_AST_MASK_SINGLE_LIST_NODES_0 ( \
    FOXY_BIT(FOXY_AST_PROGRAM)            | \
    FOXY_BIT(FOXY_AST_STMT_BLOCK)         | \
    FOXY_BIT(FOXY_AST_EXPR_ARRAY_LITERAL) | \
    FOXY_BIT(FOXY_AST_EXPR_DICT_LITERAL)    \
)
#define FOXY_AST_MASK_SINGLE_LIST_NODES_1 ((uint64_t)0)

#define FOXY_AST_MASK_LEAF_NODES_0 ( \
    FOXY_BIT(FOXY_AST_EXPR_IDENTIFIER)  | \
    FOXY_BIT(FOXY_AST_STMT_BREAK)       | \
    FOXY_BIT(FOXY_AST_STMT_CONTINUE)    | \
    FOXY_BIT(FOXY_AST_STMT_GOTO)        | \
    FOXY_BIT(FOXY_AST_STMT_LABEL)         \
)
#define FOXY_AST_MASK_LEAF_NODES_1 ((uint64_t)0)

/**
 * @brief Máscaras para clasificar métodos|miembros si están definidas al inicio.
 */
#define FOXY_AST_MASK_DECLARATIVES_0 ( \
    FOXY_BIT_0(FOX_TOKEN_KW_PRIVATE)    | \
    FOXY_BIT_0(FOX_TOKEN_KW_PROTECTED)  | \
    FOXY_BIT_0(FOX_TOKEN_KW_PUBLIC)     | \
    FOXY_BIT_0(FOX_TOKEN_MOD_PRIVATE)   | \
    FOXY_BIT_0(FOX_TOKEN_MOD_PROTECTED) | \
    FOXY_BIT_0(FOX_TOKEN_MOD_PUBLIC)      \
)

#define FOXY_AST_MASK_DECLARATIVES_1 ( \
    FOXY_BIT_1(FOX_TOKEN_KW_PRIVATE)    | \
    FOXY_BIT_1(FOX_TOKEN_KW_PROTECTED)  | \
    FOXY_BIT_1(FOX_TOKEN_KW_PUBLIC)     | \
    FOXY_BIT_1(FOX_TOKEN_MOD_PRIVATE)   | \
    FOXY_BIT_1(FOX_TOKEN_MOD_PROTECTED) | \
    FOXY_BIT_1(FOX_TOKEN_MOD_PUBLIC)      \
)

/**
 * @brief Máscaras para consumidor de calificadores global, static, const, export, private, protected, public.
 */
#define FOXY_AST_MASK_CALIFICATORS_0 ( \
    FOXY_BIT_0(FOX_TOKEN_KW_GLOBAL) | \
    FOXY_BIT_0(FOX_TOKEN_KW_STATIC) | \
    FOXY_BIT_0(FOX_TOKEN_KW_CONST)  | \
    FOXY_BIT_0(FOX_TOKEN_KW_EXPORT) | \
    FOXY_AST_MASK_DECLARATIVES_0      \
)

#define FOXY_AST_MASK_CALIFICATORS_1 ( \
    FOXY_BIT_1(FOX_TOKEN_KW_GLOBAL) | \
    FOXY_BIT_1(FOX_TOKEN_KW_STATIC) | \
    FOXY_BIT_1(FOX_TOKEN_KW_CONST)  | \
    FOXY_BIT_1(FOX_TOKEN_KW_EXPORT) | \
    FOXY_AST_MASK_DECLARATIVES_1      \
)

/* ========================================================================= */
/* INLINES DE EVALUACIÓN EN O(1)                                             */
/* ========================================================================= */

/* Helper genérico para evaluación en máscaras de 128 bits divididas */
static inline bool f_ast_match_mask(uint32_t id, uint64_t mask_0, uint64_t mask_1) {
    if (id < 64)
        return (mask_0 & ((uint64_t)1 << id)) != 0;
    if (id < 128)
        return (mask_1 & ((uint64_t)1 << (id - 64))) != 0;
    return false;
}

static inline bool f_ast_is_type_specifier(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_TYPE_SPECIFIER_TOKENS_0, FOXY_MASK_TYPE_SPECIFIER_TOKENS_1);
}

static inline bool f_ast_is_stmt_start(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_STMT_START_TOKENS_0, FOXY_MASK_STMT_START_TOKENS_1);
}

static inline bool f_ast_is_assignment_op(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_ASSIGNMENT_TOKENS_0, FOXY_MASK_ASSIGNMENT_TOKENS_1);
}

static inline bool f_ast_is_binary_op(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_BINARY_TOKENS_0, FOXY_MASK_BINARY_TOKENS_1);
}

static inline bool f_ast_is_unary_op(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_UNARY_TOKENS_0, FOXY_MASK_UNARY_TOKENS_1);
}

static inline bool f_ast_is_literal(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_MASK_LITERAL_TOKENS_0, FOXY_MASK_LITERAL_TOKENS_1);
}

static inline bool f_ast_kind_is_single_list(FoxyAstKind kind) {
    return f_ast_match_mask((uint32_t)kind, FOXY_AST_MASK_SINGLE_LIST_NODES_0, FOXY_AST_MASK_SINGLE_LIST_NODES_1);
}

static inline bool f_ast_kind_is_leaf(FoxyAstKind kind) {
    return f_ast_match_mask((uint32_t)kind, FOXY_AST_MASK_LEAF_NODES_0, FOXY_AST_MASK_LEAF_NODES_1);
}

static inline bool f_ast_is_declarative(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_AST_MASK_DECLARATIVES_0, FOXY_AST_MASK_DECLARATIVES_1);
}

static inline bool f_ast_var_is_calificator(uint32_t token_type) {
    return f_ast_match_mask(token_type, FOXY_AST_MASK_CALIFICATORS_0, FOXY_AST_MASK_CALIFICATORS_1);
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
// FOXY_EXPORT void f_ast_synchronize_parser(FoxyAstParser *parser);

#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_PRECEDENCE_LIST(F) \
    F(PREC_NONE) \
    F(PREC_ASSIGNMENT) /* = += -= */ \
    F(PREC_LAMBDA)     /* => */ \
    F(PREC_OR)         /* || */ \
    F(PREC_AND)        /* && */ \
    F(PREC_EQUALITY)   /* == != */ \
    F(PREC_COMPARISON) /* < > <= >= */ \
    F(PREC_TERM)       /* + - */ \
    F(PREC_FACTOR)     /* * / % */ \
    F(PREC_POWER)      /* ** */ \
    F(PREC_UNARY)      /* ! - ++ -- */ \
    F(PREC_CALL)       /* . () [] -> */ \
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
    PREC_LAMBDA,     // =>
    PREC_OR,         // ||
    PREC_AND,        // &&
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * / %
    PREC_POWER,      // **
    PREC_UNARY,      // ! - ++ --
    PREC_CALL,       // . () [] ->
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