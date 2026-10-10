#pragma once

#include "f_settings.h"
#include "f_lexer.h"
// #include "f_value.h"
#include "f_foxmode.h"
#include <stddef.h>
#include <stdlib.h>
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

#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_AST_VAL_LIST(F) \
    F(FOXY_AST_VAL_NULL)     \
    F(FOXY_AST_VAL_BOOL)     \
    F(FOXY_AST_VAL_INT)      \
    F(FOXY_AST_VAL_UINT)     \
    F(FOXY_AST_VAL_LONG)     \
    F(FOXY_AST_VAL_ULONG)    \
    F(FOXY_AST_VAL_LLONG)    \
    F(FOXY_AST_VAL_ULLONG)   \
    F(FOXY_AST_VAL_FLOAT)    \
    F(FOXY_AST_VAL_DOUBLE)   \
    F(FOXY_AST_VAL_LDOUBLE)  \
    F(FOXY_AST_VAL_CHAR)     \
    F(FOXY_AST_VAL_STRING)
#define F(ast_val) ast_val,
typedef enum FOXY_PACKED {
    FOXY_AST_VAL_LIST(F)
} FoxyAstValType;
#undef F
#else
typedef enum {
    FOXY_AST_VAL_NULL = 0,
    FOXY_AST_VAL_BOOL,
    FOXY_AST_VAL_INT,
    FOXY_AST_VAL_UINT,
    FOXY_AST_VAL_LONG,
    FOXY_AST_VAL_ULONG,
    FOXY_AST_VAL_LLONG,
    FOXY_AST_VAL_ULLONG,
    FOXY_AST_VAL_FLOAT,
    FOXY_AST_VAL_DOUBLE,
    FOXY_AST_VAL_LDOUBLE,
    FOXY_AST_VAL_CHAR,
    FOXY_AST_VAL_STRING
} FoxyAstValType;
#endif

typedef struct {
    FoxyAstValType type;
    union {
        bool f_bool;
        int f_int;
        unsigned int f_uint;
        long f_long;
        unsigned long f_ulong;
        long long f_llong;
        unsigned long long f_ullong;
        float f_float;
        double f_double;
        long double f_ldouble;
        char f_char;
        char *f_string; // Cadena con memoria reservada propia del AST
    } as;
} FoxyAstValue;

static inline FoxyAstValue f_ast_new_null(void) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_NULL };
}

static inline FoxyAstValue f_ast_new_bool(bool val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_BOOL, .as.f_bool = val };
}

static inline FoxyAstValue f_ast_new_int(int val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_INT, .as.f_int = val };
}

static inline FoxyAstValue f_ast_new_uint(unsigned int val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_UINT, .as.f_uint = val };
}

static inline FoxyAstValue f_ast_new_long(long val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_LONG, .as.f_long = val };
}

static inline FoxyAstValue f_ast_new_ulong(unsigned long val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_ULONG, .as.f_ulong = val };
}

static inline FoxyAstValue f_ast_new_llong(long long val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_LLONG, .as.f_llong = val };
}

static inline FoxyAstValue f_ast_new_ullong(unsigned long long val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_ULLONG, .as.f_ullong = val };
}

static inline FoxyAstValue f_ast_new_float(float val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_FLOAT, .as.f_float = val };
}

static inline FoxyAstValue f_ast_new_double(double val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_DOUBLE, .as.f_double = val };
}

static inline FoxyAstValue f_ast_new_ldouble(long double val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_LDOUBLE, .as.f_ldouble = val };
}

static inline FoxyAstValue f_ast_new_char(char val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_CHAR, .as.f_char = val };
}

static inline FoxyAstValue f_ast_new_string(char *val) {
    return (FoxyAstValue){ .type = FOXY_AST_VAL_STRING, .as.f_string = val };
}

static inline void f_ast_value_free(FoxyAstValue *val) {
    if (val && val->type == FOXY_AST_VAL_STRING && val->as.f_string) {
        free(val->as.f_string);
        val->as.f_string = NULL;
    }
}

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
    FoxyAstValue value;             /* 32 bytes o equivalente adaptado */
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
    FoxyTokenType token_type;       /* 1 byte */
    uint8_t _pad[7];                /* 7 bytes para alineación */
} FoxyAstArrayLiteral;              /* Total = 32 bytes */

typedef struct {
    FoxyAstNodeList entries;        /* 24 bytes */
} FoxyAstDictLiteral;

typedef struct {
    FoxyAstNode *value;             /* 8 bytes */
    FoxyToken key;                  /* FoxyToken */
} FoxyAstDictEntry;

/* Sentencias y Declaraciones */

typedef struct {
    FoxyAstNode *initializer;       /* 8 bytes [offset 0..7] */
    FoxyAstNode *array_size;        /* 8 bytes [offset 8..15] */
    FoxyToken name;                 /* 32 bytes [offset 16..47] */
    FoxyTokenType type_token;       /* 1 byte  [offset 48] */
    uint8_t is_array;               /* 1 byte  [offset 49] */
    uint8_t is_global;              /* 1 byte  [offset 50] */
    uint8_t is_static;              /* 1 byte  [offset 51] */
    uint8_t is_const;               /* 1 byte  [offset 52] */
    uint8_t is_hybrid;              /* 1 byte  [offset 53] */
    uint8_t _pad[2];                /* 2 bytes explícitos [offset 54..55] */
} FoxyAstVarDecl;                   /* Total = 56 bytes (Múltiplo exacto de 8) */

typedef struct {
    FoxyAstNode *body;              /* 8 bytes [offset 0..7] */
    FoxyAstNodeList params;         /* 24 bytes [offset 8..31] */
    FoxyToken name;                 /* 32 bytes [offset 32..63] */
    uint8_t is_overrule;            /* 1 byte [offset 64] */
    uint8_t _pad[7];                /* 7 bytes explícitos [offset 65..71] */
} FoxyAstFuncDecl;                  /* Total = 72 bytes (Múltiplo exacto de 8) */

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
} FoxyAstLambdaStmt;

typedef enum {
    FOXY_PREFIX_CAST,
    FOXY_PREFIX_LAMBDA
} FoxyPrefixKind;

typedef struct {
    FoxyPrefixKind kind;
    union {
        struct {
            int target_type_mask; // Máscara de bits para el tipo de destino (primitivos/numéricos)
            struct FoxyAstExpr* operand;
        } cast;
        struct {
            int return_type_mask;
            FoxyParamList* parameters;
            struct FoxyAstStmt* body; // O bloque de código de la función anónima
            bool is_anonymous;
        } lambda;
    };
} FoxyAstPrefixStmt;

/**
 * @brief Estructura Principal de Nodo AST (Tagged Union)
 */
struct FoxyAstNode {
    union {
        FoxyAstNodeList program;           /* 24 bytes */
        FoxyAstLiteral literal;            /* 32 bytes */
        FoxyAstIdentifier identifier;      /* 32 bytes */
        FoxyAstUnary unary;                /* 48 bytes */
        FoxyAstBinary binary;              /* 48 bytes */
        FoxyAstAssign assign;              /* 56 bytes */
        FoxyAstCall call;                  /* 32 bytes */
        FoxyAstGetMember get_member;       /* 40 bytes */
        FoxyAstSetMember set_member;       /* 48 bytes */
        FoxyAstGetIndex get_index;         /* 16 bytes */
        FoxyAstSetIndex set_index;         /* 24 bytes */
        FoxyAstArrayLiteral array_literal; /* 24 bytes */
        FoxyAstDictLiteral dict_literal;   /* 24 bytes */
        FoxyAstDictEntry dict_entry;       /* 40 bytes */
        FoxyAstNode *expr_stmt;            /* 8 bytes */
        FoxyAstVarDecl var_decl;           /* 56 bytes */
        FoxyAstFuncDecl func_decl;         /* 72 bytes */
        FoxyAstBlockStmt block_stmt;       /* 24 bytes */
        FoxyAstIfStmt if_stmt;             /* 24 bytes */
        FoxyAstWhileStmt while_stmt;       /* 16 bytes */
        FoxyAstForStmt for_stmt;           /* 32 bytes */
        FoxyAstForeachStmt foreach_stmt;   /* 48 bytes */
        FoxyAstSwitchStmt switch_stmt;     /* 32 bytes */
        FoxyAstCaseStmt case_stmt;         /* 32 bytes */
        FoxyAstReturnStmt return_stmt;     /* 8 bytes */
        FoxyAstGotoStmt goto_stmt;         /* 32 bytes */
        FoxyAstTryStmt try_stmt;           /* 40 bytes */
        FoxyAstCatchStmt catch_stmt;       /* 40 bytes */
        FoxyAstIncludeStmt include_stmt;   /* 32 bytes */
        FoxyAstLambdaStmt lambda_stmt;     /* 32 bytes */
        
        /* Forzar la alineación y tamaño de la union a un múltiplo exacto de 8 bytes. */
        uint64_t _align_union[10];         /* Reserva espacio alineado a 80 bytes */
    } as;

    FoxySourcePos pos;                     /* 16 bytes (const char* [8] + uint32_t [4] + uint32_t [4]) */
    FoxyAstKind kind;                      /* 1 byte */
    uint8_t _pad[15];                      /* 15 bytes para completar alineación de 8 bytes */
};

/* ============================================================================ */
/* MÁSCARAS BITWISE UNIFICADAS DE 128 BITS (UTILIZA FOXY_BIT128 DE f_foxmode.h) */
/* ============================================================================ */

#define FOXY_AST_MASK_SINGLE_LIST_NODES FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOXY_AST_PROGRAM)            | \
    FOXY_TOK_BIT_W0(FOXY_AST_STMT_BLOCK)         | \
    FOXY_TOK_BIT_W0(FOXY_AST_EXPR_ARRAY_LITERAL) | \
    FOXY_TOK_BIT_W0(FOXY_AST_EXPR_DICT_LITERAL),   \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

#define FOXY_AST_MASK_LEAF_NODES FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOXY_AST_EXPR_IDENTIFIER) | \
    FOXY_TOK_BIT_W0(FOXY_AST_STMT_BREAK)      | \
    FOXY_TOK_BIT_W0(FOXY_AST_STMT_CONTINUE)   | \
    FOXY_TOK_BIT_W0(FOXY_AST_STMT_GOTO)       | \
    FOXY_TOK_BIT_W0(FOXY_AST_STMT_LABEL),        \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/* ========================================================================= */
/* INLINES DE EVALUACIÓN Y PREDICADOS                                        */
/* ========================================================================= */

static inline bool f_ast_match_mask128(uint32_t id, foxy_mask_t mask) {
    return foxy_mask_contains(mask, id);
}

static inline bool f_ast_is_type_specifier(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_TYPE_SPECIFIERS);
}

static inline bool f_ast_is_stmt_start(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_STMT_START_TOKENS);
}

static inline bool f_ast_is_assignment_op(uint32_t token_type) {
    return f_token_is_assignment(token_type);
}

static inline bool f_ast_is_binary_op(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_BINARY_TOKENS);
}

static inline bool f_ast_is_unary_op(uint32_t token_type) {
    return f_token_is_unary(token_type);
}

static inline bool f_ast_is_literal(uint32_t token_type) {
    return f_token_is_primary(token_type);
}

static inline bool f_ast_kind_is_single_list(FoxyAstKind kind) {
    return f_ast_match_mask128((uint32_t)kind, FOXY_AST_MASK_SINGLE_LIST_NODES);
}

static inline bool f_ast_kind_is_leaf(FoxyAstKind kind) {
    return f_ast_match_mask128((uint32_t)kind, FOXY_AST_MASK_LEAF_NODES);
}

static inline bool f_ast_is_declarative(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_DECLARATIVE_TOKENS);
}

static inline bool f_ast_var_is_calificator(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_QUALIFIER_TOKENS);
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
FOXY_EXPORT FoxyAstValue f_ast_value_from_token(const FoxyToken *token);
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