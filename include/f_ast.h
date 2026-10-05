#pragma once

#include "f_settings.h"
#include "f_lexer.h"
#include "f_value.h"
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

/** Macro auxiliar para verificar si un puntero a nodo AST es de categoría de Expresión */
#define FOXY_AST_IS_EXPR_KIND(kind) \
    ((kind) >= FOXY_AST_EXPR_LITERAL && (kind) <= FOXY_AST_EXPR_DICT_ENTRY)

/** Macro auxiliar para verificar si un puntero a nodo AST es de categoría de Sentencia */
#define FOXY_AST_IS_STMT_KIND(kind) \
    ((kind) >= FOXY_AST_STMT_EXPR && (kind) <= FOXY_AST_STMT_CATCH)

/**
 * @brief Lista dinámica de nodos AST para almacenar bloques, argumentos o parámetros.
 */
typedef struct {
    FoxyAstNode **nodes;
    size_t count;
    size_t capacity;
} FoxyAstNodeList;

/* ========================================================================= */
/* DATOS ESPECÍFICOS SEGÚN LA CATEGORÍA DEL NODO                             */
/* ========================================================================= */

typedef struct {
    FoxyValue value;                /* Valor parseado en compilación (Int, Double, String, etc.) */
} FoxyAstLiteral;

typedef struct {
    FoxyToken name;                 /* Token del identificador */
} FoxyAstIdentifier;

typedef struct {
    FoxyToken op;                   /* Operador (!, -, ~, #, ++, --, etc.) */
    FoxyAstNode *operand;           /* Expresión a la que se le aplica */
    bool is_postfix;                /* true si es a++, false si es ++a */
} FoxyAstUnary;

typedef struct {
    FoxyToken op;                   /* Operador (+, -, *, /, ==, !=, &&, .., etc.) */
    FoxyAstNode *left;
    FoxyAstNode *right;
} FoxyAstBinary;

typedef struct {
    FoxyAstNode *target;            /* Identificador, Acceso a miembro o Índice */
    FoxyToken op;                   /* =, +=, -=, *=, /=, etc. */
    FoxyAstNode *value;
} FoxyAstAssign;

typedef struct {
    FoxyAstNode *callee;            /* Identificador o expresión evaluable a función */
    FoxyAstNodeList args;           /* Argumentos pasados a la llamada */
} FoxyAstCall;

typedef struct {
    FoxyAstNode *object;            /* Objeto al que se accede (ej: out.printf) */
    FoxyToken member;               /* Identificador del miembro */
} FoxyAstGetMember;

typedef struct {
    FoxyAstNode *object;
    FoxyToken member;
    FoxyAstNode *value;
} FoxyAstSetMember;

typedef struct {
    FoxyAstNode *target;            /* Arreglo o Diccionario */
    FoxyAstNode *index;             /* Expresión del índice o clave */
} FoxyAstGetIndex;

typedef struct {
    FoxyAstNode *target;
    FoxyAstNode *index;
    FoxyAstNode *value;
} FoxyAstSetIndex;

typedef struct {
    FoxyAstNodeList elements;       /* Elementos de la lista [a, b, c] */
} FoxyAstArrayLiteral;

typedef struct {
    FoxyAstNodeList entries;        /* Nodos de tipo FOXY_AST_EXPR_DICT_ENTRY */
} FoxyAstDictLiteral;

typedef struct {
    FoxyToken key;                  /* Clave (identificador o literal) */
    FoxyAstNode *value;             /* Valor asignado */
} FoxyAstDictEntry;

/* Sentencias y Declaraciones */

typedef struct {
    FoxyToken name;                 /* Nombre de la variable */
    FoxyTokenType type_token;       /* Tipo explícito si lo tiene (ej: FOX_TOKEN_KW_INT), o FOX_TOKEN_ERROR si es inferido */
    FoxyAstNode *initializer;       /* Expresión inicial de asignación (opcional, puede ser NULL) */
} FoxyAstVarDecl;

typedef struct {
    FoxyToken name;                 /* Nombre de la función (o token nulo si es anónima) */
    FoxyAstNodeList params;         /* Lista de identificadores o parámetros decl */
    FoxyAstNode *body;              /* Nodo de tipo FOXY_AST_STMT_BLOCK */
} FoxyAstFuncDecl;

typedef struct {
    FoxyAstNode *condition;
    FoxyAstNode *then_branch;       /* Bloque / Sentencia ejecutable si condition == true */
    FoxyAstNode *else_branch;       /* Bloque / Sentencia o FOXY_AST_STMT_IF en caso de elseif (opcional) */
} FoxyAstIfStmt;

typedef struct {
    FoxyAstNode *condition;
    FoxyAstNode *body;
} FoxyAstWhileStmt;

typedef struct {
    FoxyAstNode *init;              /* Decl/Expr inicial (ej: int i = 0), puede ser NULL */
    FoxyAstNode *condition;         /* Condición de iteración (ej: i < 10), puede ser NULL */
    FoxyAstNode *increment;         /* Expresión de paso (ej: i++), puede ser NULL */
    FoxyAstNode *body;
} FoxyAstForStmt;

typedef struct {
    FoxyToken iterator_var;         /* Variable de iteración */
    FoxyAstNode *iterable;          /* Expresión a iterar (Arreglo/Diccionario) */
    FoxyAstNode *body;
} FoxyAstForeachStmt;

typedef struct {
    FoxyAstNode *condition;         /* Expresión del switch */
    FoxyAstNodeList cases;          /* Lista de casos (FOXY_AST_STMT_CASE) */
} FoxyAstSwitchStmt;

typedef struct {
    FoxyAstNode *expr;              /* Expresión del case (NULL si es 'default') */
    FoxyAstNodeList stmts;          /* Sentencias a ejecutar dentro del case */
} FoxyAstCaseStmt;

typedef struct {
    FoxyAstNode *value;             /* Expresión de retorno (opcional, NULL si retorna void) */
} FoxyAstReturnStmt;

typedef struct {
    FoxyToken label;                /* Etiqueta de salto */
} FoxyAstGotoStmt;

typedef struct {
    FoxyAstNode *try_block;
    FoxyAstNodeList catch_blocks;   /* Lista de bloques catch */
    FoxyAstNode *finally_block;     /* Bloque final (opcional, NULL si se omite) */
} FoxyAstTryStmt;

typedef struct {
    FoxyToken var_name;             /* Variable de captura de excepción */
    FoxyAstNode *body;
} FoxyAstCatchStmt;

typedef struct {
    FoxyLexer lexer;
    FoxyToken current_token;
    FoxyToken previous_token;
    bool had_error;
    bool panic_mode;
} FoxyAstParser;

/**
 * @brief Estrutura Principal de Nodo AST (Tagged Union)
 */
struct FoxyAstNode {
    FoxyAstKind kind;
    FoxySourcePos pos;              /* Ubicación exacta tomada del token correspondiente */

    union {
        /* Válido si kind == FOXY_AST_PROGRAM o FOXY_AST_STMT_BLOCK */
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

        /* Válido si kind == FOXY_AST_STMT_EXPR */
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
    } as;
};

/* ========================================================================= */
/* API DE CONSTRUCCIÓN, GESTIÓN Y LIMPIEZA DE NODOS AST                      */
/* ========================================================================= */

/**
 * @brief Asigna e inicializa un nuevo nodo del AST.
 */
FOXY_EXPORT FoxyAstNode *f_ast_create_node(FoxyAstKind kind, FoxySourcePos pos);

/**
 * @brief Inicializa una lista dinámica de nodos AST.
 */
FOXY_EXPORT void f_ast_node_list_init(FoxyAstNodeList *list);

/**
 * @brief Inserta un nuevo nodo al final de una lista dinámica AST.
 */
FOXY_EXPORT void f_ast_node_list_append(FoxyAstNodeList *list, FoxyAstNode *node);

/**
 * @brief Libera la memoria consumida por una lista de nodos sin liberar sus nodos hijos.
 */
FOXY_EXPORT void f_ast_node_list_free_shallow(FoxyAstNodeList *list);

/**
 * @brief Libera recursivamente toda la memoria asignada a un nodo AST y a todos sus sub-árboles.
 */
FOXY_EXPORT void f_ast_free_node(FoxyAstNode *node);

/**
 * @brief Helper de depuración para visualizar la jerarquía del AST en stdout.
 */
FOXY_EXPORT void f_ast_print(const FoxyAstNode *node, int indent);

/**
 * @brief Devuelve el nombre en cadena de texto estático del kind de un nodo.
 */
FOXY_EXPORT const char *f_ast_kind_to_string(FoxyAstKind kind);

/**
 * @brief Helper de duplicación de cadenas seguras para el AST
 */
FOXY_EXPORT char *f_ast_strdup(const char *src, size_t length);

/**
 * @brief Inicialización del parser dentro del módulo f_ast
 */
FOXY_EXPORT void f_ast_parser_init(FoxyAstParser *parser, FILE *file, const char *filename);

/**
 * @brief Prototipo externo corregido bajo el dominio f_ast
 */
FOXY_EXPORT FoxyAstNode *f_ast_parse_program(FoxyAstParser *parser);

/**
 * @brief Convierte un FoxyToken de tipo literal (FOX_TOKEN_*_LITERAL, KW_TRUE, etc.)
 *        a su estructura FoxyValue correspondiente usando los constructores f_value_new_*.
 */
FOXY_EXPORT FoxyValue f_ast_value_from_token(const FoxyToken *token);