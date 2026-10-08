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
static void f_ast_error_at_current(FoxyAstParser *parser, const char *message);

static FoxyAstNode *f_ast_parse_include(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_var_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_statement(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence);
static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type);
static FoxyAstNode *f_ast_parse_expression_statement(FoxyAstParser *parser);

/* Reglas de Parseo (deben coincidir con FoxyParseFn: parser, left, can_assign) */
static FoxyAstNode *f_ast_parse_block_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
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
static FoxyAstNode *f_ast_parse_if_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_while_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_for_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_foreach_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_switch_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_case_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_break_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_continue_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_goto_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_try_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_return_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_function_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_unpack(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_export_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_enum_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_use_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_class_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_struct_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);

static const FoxyParseRule rules[] = {
    [FOX_TOKEN_EOF]              = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_ERROR]            = { NULL,                NULL,              PREC_NONE },

    /* Identificadores y Literales */
    [FOX_TOKEN_IDENTIFIER]       = { f_ast_parse_variable,NULL,              PREC_NONE },
    [FOX_TOKEN_LABEL]            = { NULL,                NULL,              PREC_NONE },
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
    [FOX_TOKEN_KW_GLOBAL]        = { f_ast_parse_var_declaration,NULL,       PREC_NONE },
    [FOX_TOKEN_KW_STATIC]        = { f_ast_parse_var_declaration,NULL,       PREC_NONE },
    [FOX_TOKEN_KW_CONST]         = { f_ast_parse_var_declaration,NULL,       PREC_NONE },
    [FOX_TOKEN_KW_NULL]          = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_BOOL]          = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_CHAR]          = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_UCHAR]         = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_SHORT]         = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_USHORT]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_INT]           = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_UINT]          = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_LONG]          = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_ULONG]         = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_LLONG]         = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_ULLONG]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_FLOAT]         = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_DOUBLE]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_LDOUBLE]       = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_NUMBER]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_DICT]          = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_OBJECT]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },

    /* Control de Flujo */
    [FOX_TOKEN_KW_IF]            = { f_ast_parse_if_statement,       NULL, PREC_NONE },
    [FOX_TOKEN_KW_ELSEIF]        = { f_ast_parse_if_statement,       NULL, PREC_NONE },
    [FOX_TOKEN_KW_ELSE]          = { NULL,                           NULL, PREC_NONE },
    [FOX_TOKEN_KW_WHILE]         = { f_ast_parse_while_statement,    NULL, PREC_NONE },
    [FOX_TOKEN_KW_FOR]           = { f_ast_parse_for_statement,      NULL, PREC_NONE },
    [FOX_TOKEN_KW_FOREACH]       = { f_ast_parse_foreach_statement,  NULL, PREC_NONE },
    [FOX_TOKEN_KW_SWITCH]        = { f_ast_parse_switch_statement,   NULL, PREC_NONE },
    [FOX_TOKEN_KW_CASE]          = { f_ast_parse_case_statement,     NULL, PREC_NONE },
    [FOX_TOKEN_KW_DEFAULT]       = { f_ast_parse_case_statement,     NULL, PREC_NONE },
    [FOX_TOKEN_KW_RETURN]        = { f_ast_parse_return_statement,   NULL, PREC_NONE },
    [FOX_TOKEN_KW_BREAK]         = { f_ast_parse_break_statement,    NULL, PREC_NONE },
    [FOX_TOKEN_KW_CONTINUE]      = { f_ast_parse_continue_statement, NULL, PREC_NONE },
    [FOX_TOKEN_KW_GOTO]          = { f_ast_parse_goto_statement,     NULL, PREC_NONE },
    [FOX_TOKEN_KW_TRY]           = { f_ast_parse_try_statement,      NULL, PREC_NONE },
    [FOX_TOKEN_KW_CATCH]         = { NULL,                           NULL, PREC_NONE },
    [FOX_TOKEN_KW_EXCEPT]        = { NULL,                           NULL, PREC_NONE },
    [FOX_TOKEN_KW_FINAL]         = { NULL,                           NULL, PREC_NONE },

    /* POO y Estructuras */
    [FOX_TOKEN_KW_CLASS]         = { f_ast_parse_class_declaration,  NULL,   PREC_NONE },
    [FOX_TOKEN_KW_STRUCT]        = { f_ast_parse_struct_declaration, NULL,   PREC_NONE },
    [FOX_TOKEN_KW_ENUM]          = { f_ast_parse_enum_declaration,NULL,      PREC_NONE },
    [FOX_TOKEN_KW_FROM]          = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FUNCTION]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_KW_OVERRULE]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_INCLUDE]       = { f_ast_parse_include, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_USE]           = { f_ast_parse_use_statement,NULL,         PREC_NONE },
    [FOX_TOKEN_KW_EXPORT]        = { f_ast_parse_export_statement,NULL,      PREC_NONE },
    [FOX_TOKEN_KW_SUPER]         = { f_ast_parse_super,   NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SELF]          = { f_ast_parse_self,    NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ANCESTOROF]    = { NULL,         f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_KW_DESCENDANTOF]  = { NULL,         f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_KW_PARENTOF]      = { NULL,         f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_KW_CHILDOF]       = { NULL,         f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_KW_TYPEOF]        = { f_ast_parse_unary,   NULL,              PREC_UNARY },
    [FOX_TOKEN_KW_TRUE]          = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FALSE]         = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PRIVATE]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PROTECTED]     = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_PUBLIC]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PRIVATE]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PROTECTED]    = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_MOD_PUBLIC]       = { NULL,                NULL,              PREC_NONE },

    /* Métodos Marcados */
    [FOX_TOKEN_METHOD_NEW]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CAST]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_TOSTRING]  = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_ADD]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_SUB]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_MUL]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_DIV]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_POW]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_MOD]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CONCAT]    = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_UNM]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_NOT]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_EQ]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_NEQ]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LT]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_GT]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LE]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_GE]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BAND]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BOR]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BNOT]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_BXOR]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LSHIFT]    = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_RSHIFT]    = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_FOREACH]   = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_CLOSED]    = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_METHOD_LEN]       = { NULL,                NULL,              PREC_NONE },

    /* Operadores Aritméticos, Lógicos, Bitwise y Asignaciones */
    [FOX_TOKEN_PLUS]             = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_MINUS]            = { f_ast_parse_unary,  f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_STAR]             = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_SLASH]            = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_PERCENT]          = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_POWER]            = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_HASH]             = { f_ast_parse_unary,  NULL,               PREC_UNARY },
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
    [FOX_TOKEN_BANG]             = { f_ast_parse_unary,  NULL,               PREC_UNARY },
    [FOX_TOKEN_AND]              = { NULL,               f_ast_parse_binary, PREC_AND },
    [FOX_TOKEN_OR]               = { NULL,               f_ast_parse_binary, PREC_OR },
    [FOX_TOKEN_AMPERSAND]        = { f_ast_parse_unary,  f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_PIPE]             = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_TILDE]            = { f_ast_parse_unary,  NULL,               PREC_UNARY },
    [FOX_TOKEN_CARET]            = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_LSHIFT]           = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_RSHIFT]           = { NULL,               f_ast_parse_binary, PREC_TERM },

    /* Delimitadores y Puntuación */
    [FOX_TOKEN_ARROW]            = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_FAT_ARROW]        = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_PTR_ARROW]        = { NULL,               f_ast_parse_member, PREC_CALL },
    [FOX_TOKEN_ELLIPSIS]         = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_LPAREN]           = { f_ast_parse_grouping,f_ast_parse_call,  PREC_CALL },
    [FOX_TOKEN_RPAREN]           = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_LBRACE]           = { f_ast_parse_dict,   NULL,               PREC_NONE },
    [FOX_TOKEN_RBRACE]           = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_LBRACKET]         = { f_ast_parse_array,  f_ast_parse_index,  PREC_CALL },
    [FOX_TOKEN_RBRACKET]         = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_SEMICOLON]        = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_COLON]            = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_COMMA]            = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_DOT]              = { NULL,               f_ast_parse_member, PREC_CALL },
    [FOX_TOKEN_DOTDOT]           = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_DOTDOTDOT]        = { f_ast_parse_unpack, NULL,               PREC_UNARY },
    [FOX_TOKEN_QUESTION]         = { NULL,               f_ast_parse_ternary,PREC_ASSIGNMENT }
};

static inline bool f_ast_is_valid_func_name(FoxyTokenType type) {
    return type == FOX_TOKEN_IDENTIFIER || (type >= FOX_TOKEN_METHOD_NEW && type <= FOX_TOKEN_METHOD_LEN);
}

static inline bool f_ast_is_var_decl_start(FoxyTokenType type) {
    return type == FOX_TOKEN_KW_GLOBAL ||
           type == FOX_TOKEN_KW_STATIC ||
           type == FOX_TOKEN_KW_CONST  ||
           f_ast_is_type_specifier(type);
}

static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type) {
    return &rules[type];
}

static FoxyAstNode *f_ast_parse_expression_statement(FoxyAstParser *parser) {
    FoxySourcePos pos = parser->current_token.pos;
    FoxyAstNode *expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    // Consumir ';' opcional si está presente
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_EXPR, pos);
    node->as.expr_stmt = expr;
    return node;
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

    // Manejo de tuplas de tipos o firmas de función/lambda: (int, int)
    if (f_ast_is_type_specifier(parser->current_token.type)) {
        FoxyToken type_token = parser->current_token;
        f_ast_advance(parser);

        if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
            f_ast_advance(parser);
            if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                f_ast_advance(parser);
            }
        }

        while (parser->current_token.type == FOX_TOKEN_COMMA) {
            f_ast_advance(parser);
            if (f_ast_is_type_specifier(parser->current_token.type)) {
                f_ast_advance(parser);
            }
        }

        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "Expected ')' after type list.");
            return NULL;
        }

        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
            return f_ast_parse_var_declaration(parser, NULL, false);
        }

        FoxyAstNode *operand = f_ast_parse_precedence(parser, PREC_UNARY);
        FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, type_token.pos);
        node->as.unary.op = type_token;
        node->as.unary.operand = operand;
        node->as.unary.is_postfix = 0;
        return node;
    }

    FoxyAstNode *expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected ')' after grouping expression.");
    }

    if (expr != NULL && expr->kind == FOXY_AST_EXPR_ASSIGN) {
        expr->as.assign.is_grouped = 1;
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
L_FREE_FOXY_AST_STMT_INCLUDE:
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
    if (!list || !list->nodes) return;
    for (size_t i = 0; i < list->count; ++i) {
        if (list->nodes[i]) {
            f_ast_print(list->nodes[i], indent);
        }
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
            printf("Identifier: %.*s\n", 
                   node->as.identifier.name.length, 
                   node->as.identifier.name.start);
            break;

        case FOXY_AST_EXPR_UNARY:
            print_indent(indent + 1);
            printf("Op: %.*s (postfix: %s)\n", 
                   node->as.unary.op.length, 
                   node->as.unary.op.start,
                   node->as.unary.is_postfix ? "true" : "false");
            if (node->as.unary.operand) {
                f_ast_print(node->as.unary.operand, indent + 1);
            }
            break;

        case FOXY_AST_EXPR_BINARY:
            print_indent(indent + 1);
            printf("Op: %.*s\n", node->as.binary.op.length, node->as.binary.op.start);
            if (node->as.binary.left)  f_ast_print(node->as.binary.left, indent + 1);
            if (node->as.binary.right) f_ast_print(node->as.binary.right, indent + 1);
            break;

        case FOXY_AST_EXPR_ASSIGN:
            print_indent(indent + 1);
            printf("Op: %.*s (grouped: %s)\n", 
                   node->as.assign.op.length, 
                   node->as.assign.op.start,
                   node->as.assign.is_grouped ? "true" : "false");
            if (node->as.assign.target) f_ast_print(node->as.assign.target, indent + 1);
            if (node->as.assign.value)  f_ast_print(node->as.assign.value, indent + 1);
            break;

        case FOXY_AST_EXPR_CALL:
            if (node->as.call.callee) {
                f_ast_print(node->as.call.callee, indent + 1);
            }
            print_node_list(&node->as.call.args, indent + 1);
            break;

        case FOXY_AST_EXPR_GET_MEMBER:
            print_indent(indent + 1);
            printf("Member: %.*s\n", 
                   node->as.get_member.member.length, 
                   node->as.get_member.member.start);
            if (node->as.get_member.object) {
                f_ast_print(node->as.get_member.object, indent + 1);
            }
            break;

        case FOXY_AST_EXPR_SET_MEMBER:
            print_indent(indent + 1);
            printf("Member: %.*s\n", 
                   node->as.set_member.member.length, 
                   node->as.set_member.member.start);
            if (node->as.set_member.object) f_ast_print(node->as.set_member.object, indent + 1);
            if (node->as.set_member.value)  f_ast_print(node->as.set_member.value, indent + 1);
            break;

        case FOXY_AST_EXPR_GET_INDEX:
            if (node->as.get_index.target) f_ast_print(node->as.get_index.target, indent + 1);
            if (node->as.get_index.index)  f_ast_print(node->as.get_index.index, indent + 1);
            break;

        case FOXY_AST_EXPR_SET_INDEX:
            if (node->as.set_index.target) f_ast_print(node->as.set_index.target, indent + 1);
            if (node->as.set_index.index)  f_ast_print(node->as.set_index.index, indent + 1);
            if (node->as.set_index.value)  f_ast_print(node->as.set_index.value, indent + 1);
            break;

        case FOXY_AST_EXPR_DICT_ENTRY:
            print_indent(indent + 1);
            printf("Key: %.*s\n", 
                   node->as.dict_entry.key.length, 
                   node->as.dict_entry.key.start);
            if (node->as.dict_entry.value) {
                f_ast_print(node->as.dict_entry.value, indent + 1);
            }
            break;

        case FOXY_AST_STMT_EXPR:
            if (node->as.expr_stmt) {
                f_ast_print(node->as.expr_stmt, indent + 1);
            }
            break;

        case FOXY_AST_STMT_VAR_DECL:
            print_indent(indent + 1);
            printf("Var: %.*s (type_token: %d)\n", 
                   node->as.var_decl.name.length, 
                   node->as.var_decl.name.start, 
                   node->as.var_decl.type_token);
            if (node->as.var_decl.initializer) {
                f_ast_print(node->as.var_decl.initializer, indent + 1);
            }
            break;

        case FOXY_AST_STMT_FUNC_DECL:
            print_indent(indent + 1);
            printf("Function: %.*s\n", 
                   node->as.func_decl.name.length, 
                   node->as.func_decl.name.start);
            print_node_list(&node->as.func_decl.params, indent + 1);
            if (node->as.func_decl.body) {
                f_ast_print(node->as.func_decl.body, indent + 1);
            }
            break;

        case FOXY_AST_STMT_IF:
            if (node->as.if_stmt.condition)   f_ast_print(node->as.if_stmt.condition, indent + 1);
            if (node->as.if_stmt.then_branch) f_ast_print(node->as.if_stmt.then_branch, indent + 1);
            if (node->as.if_stmt.else_branch) f_ast_print(node->as.if_stmt.else_branch, indent + 1);
            break;

        case FOXY_AST_STMT_WHILE:
            if (node->as.while_stmt.condition) f_ast_print(node->as.while_stmt.condition, indent + 1);
            if (node->as.while_stmt.body)      f_ast_print(node->as.while_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_FOR:
            if (node->as.for_stmt.init)      f_ast_print(node->as.for_stmt.init, indent + 1);
            if (node->as.for_stmt.condition) f_ast_print(node->as.for_stmt.condition, indent + 1);
            if (node->as.for_stmt.increment) f_ast_print(node->as.for_stmt.increment, indent + 1);
            if (node->as.for_stmt.body)      f_ast_print(node->as.for_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_FOREACH:
            print_indent(indent + 1);
            printf("Iterator: %.*s\n", 
                   node->as.foreach_stmt.iterator_var.length, 
                   node->as.foreach_stmt.iterator_var.start);
            if (node->as.foreach_stmt.iterable) f_ast_print(node->as.foreach_stmt.iterable, indent + 1);
            if (node->as.foreach_stmt.body)     f_ast_print(node->as.foreach_stmt.body, indent + 1);
            break;

        case FOXY_AST_STMT_SWITCH:
            if (node->as.switch_stmt.condition) {
                f_ast_print(node->as.switch_stmt.condition, indent + 1);
            }
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
            printf("Goto Target: %.*s\n", 
                   node->as.goto_stmt.label.length, 
                   node->as.goto_stmt.label.start);
            break;

        case FOXY_AST_STMT_TRY:
            if (node->as.try_stmt.try_block) {
                f_ast_print(node->as.try_stmt.try_block, indent + 1);
            }
            print_node_list(&node->as.try_stmt.catch_blocks, indent + 1);
            if (node->as.try_stmt.finally_block) {
                f_ast_print(node->as.try_stmt.finally_block, indent + 1);
            }
            break;

        case FOXY_AST_STMT_CATCH:
            print_indent(indent + 1);
            printf("Catch Var: %.*s\n", 
                   node->as.catch_stmt.var_name.length, 
                   node->as.catch_stmt.var_name.start);
            if (node->as.catch_stmt.body) {
                f_ast_print(node->as.catch_stmt.body, indent + 1);
            }
            break;

        case FOXY_AST_STMT_INCLUDE:
            print_indent(indent + 1);
            printf("Include Path: %.*s\n", 
                node->as.include_stmt.path.length, 
                node->as.include_stmt.path.start);
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

    switch (token->type) {
        case FOX_TOKEN_KW_TRUE:
            return f_value_new_bool(true);
        case FOX_TOKEN_KW_FALSE:
            return f_value_new_bool(false);
        case FOX_TOKEN_KW_NULL:
            return f_value_new_null();
        case FOX_TOKEN_INT_LITERAL:
            return f_value_new_int((int)strtol(token->start, NULL, 10));
        case FOX_TOKEN_UINT_LITERAL:
            return f_value_new_uint((unsigned int)strtoul(token->start, NULL, 10));
        case FOX_TOKEN_LONG_LITERAL:
            return f_value_new_long(strtol(token->start, NULL, 10));
        case FOX_TOKEN_ULONG_LITERAL:
            return f_value_new_ulong(strtoul(token->start, NULL, 10));
        case FOX_TOKEN_LLONG_LITERAL:
            return f_value_new_llong(strtoll(token->start, NULL, 10));
        case FOX_TOKEN_ULLONG_LITERAL:
            return f_value_new_ullong(strtoull(token->start, NULL, 10));
        case FOX_TOKEN_FLOAT_LITERAL:
            return f_value_new_float((float)strtod(token->start, NULL));
        case FOX_TOKEN_DOUBLE_LITERAL:
            return f_value_new_double(strtod(token->start, NULL));
        case FOX_TOKEN_LDOUBLE_LITERAL:
            return f_value_new_ldouble(strtold(token->start, NULL));
        case FOX_TOKEN_NUMBER_LITERAL:
            return f_value_new_double(strtod(token->start, NULL));
        case FOX_TOKEN_CHAR_LITERAL:
            return f_value_new_char(token->length > 1 ? token->start[1] : 0);
        default:
            return f_value_new_null();
    }
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

FoxyAstNode *f_ast_create_binary_node(FoxyToken op, FoxyAstNode *left, FoxyAstNode *right) {
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_BINARY, op.pos);
    if (!node) return NULL;
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
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

/**
 * @brief Descarta tokens hasta encontrar un punto de sincronización seguro (Boundary)
 */

static void f_ast_synchronize(FoxyAstParser *parser) {
    parser->panic_mode = false;

    // Forzar el avance de al menos un token para romper bucles repetidos
    f_ast_advance(parser);

    while (parser->current_token.type != FOX_TOKEN_EOF) {
        if (parser->previous_token.type == FOX_TOKEN_SEMICOLON) {
            return;
        }

        if (f_ast_is_stmt_start(parser->current_token.type) ||
            f_ast_is_var_decl_start(parser->current_token.type)) {
            return;
        }

        f_ast_advance(parser);
    }
}

static void f_ast_error_at_current(FoxyAstParser *parser, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    fprintf(stderr, "[Foxy Parser Error] Line %u: %s\n", parser->current_token.pos.line, message);
}

FoxyAstNode *f_ast_parse_program(FoxyAstParser *parser) {
    if (!parser) return NULL;

    FoxyAstNode *program_node = f_ast_create_node(FOXY_AST_PROGRAM, parser->current_token.pos);
    if (!program_node) {
        parser->had_error = true;
        return NULL;
    }

    while (parser->current_token.type != FOX_TOKEN_EOF) {
        FoxyTokenType prev_type = parser->current_token.type;
        uint32_t prev_line = parser->current_token.pos.line;
        uint32_t prev_col  = parser->current_token.pos.column;

        FoxyAstNode *stmt = f_ast_parse_declaration(parser);

        if (stmt != NULL) {
            f_ast_append_child(program_node, stmt);
        } else {
            f_ast_synchronize(parser);
        }

        // Guarda anti-bucle: si el parser quedó varado en el mismo token exacto, forzar f_ast_advance
        if (parser->current_token.type == prev_type &&
            parser->current_token.pos.line == prev_line &&
            parser->current_token.pos.column == prev_col) {
            f_ast_advance(parser);
        }
    }

    if (parser->had_error) {
        f_ast_free_node(program_node);
        return NULL;
    }

    return program_node;
}

static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence) {
    // 1. Consume el token prefix (actual -> previous)
    f_ast_advance(parser);

    FoxyParseFn prefix_rule = f_ast_get_rule(parser->previous_token.type)->prefix;
    if (prefix_rule == NULL) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_precedence] Expected expression.");
        return NULL;
    }

    FoxyAstNode *node = prefix_rule(parser, NULL, precedence <= PREC_ASSIGNMENT);

    // 2. Bucle para infix/postfix (verificando que infix != NULL)
    while (precedence <= f_ast_get_rule(parser->current_token.type)->precedence) {
        FoxyParseFn infix_rule = f_ast_get_rule(parser->current_token.type)->infix;
        if (infix_rule == NULL) {
            break; // Salir si el token no define un operador infijo/postfijo válido
        }

        f_ast_advance(parser); // Consume el operador infijo
        node = infix_rule(parser, node, precedence <= PREC_ASSIGNMENT);
        
        if (node == NULL) {
            break;
        }
    }

    return node;
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
    FoxyToken op = parser->previous_token;
    const FoxyParseRule *rule = f_ast_get_rule(op.type);

    FoxyAstNode *right = f_ast_parse_precedence(parser, (FoxyPrecedence)(rule->precedence + 1));
    if (!right) {
        f_ast_free_node(left); // Evita fugas de memoria si falla el lado derecho
        return NULL;
    }

    return f_ast_create_binary_node(op, left, right);
}

static FoxyAstNode *f_ast_parse_statement(FoxyAstParser *parser) {
    if (parser->panic_mode) f_ast_synchronize(parser);

    // Detección de etiqueta de salto <miEtiqueta>
    if (parser->current_token.type == FOX_TOKEN_LABEL) {
        FoxyToken label_token = parser->current_token;
        f_ast_advance(parser);

        FoxyAstNode *label_node = f_ast_create_node(FOXY_AST_STMT_LABEL, label_token.pos);
        label_node->as.goto_stmt.label = label_token;
        return label_node;
    }

    // Caso de bloque de sentencias
    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, NULL);
    }

    // Palabras clave con sentencias dedicadas (if, while, for, return, break, etc.)
    if (f_ast_is_stmt_start(parser->current_token.type)) {
        const FoxyParseRule *rule = f_ast_get_rule(parser->current_token.type);
        if (rule && rule->prefix != NULL) {
            return rule->prefix(parser, NULL, false);
        }
    }

    // Para identificadores, expresiones y asignaciones directas
    return f_ast_parse_expression_statement(parser);
}

static FoxyAstNode *f_ast_parse_include(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->current_token.pos;

    // Consumir la palabra clave 'include'
    f_ast_advance(parser);

    if (parser->current_token.type != FOX_TOKEN_STRING_LITERAL &&
        parser->current_token.type != FOX_TOKEN_IDENTIFIER) {
        f_ast_error_at_current(parser, "Expected string literal or module path after 'include'.");
        return NULL;
    }

    FoxyToken path_token = parser->current_token;
    f_ast_advance(parser); // Consumir la cadena/identificador de ruta

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_INCLUDE, pos);
    node->as.include_stmt.path = path_token;
    return node;
}

static bool f_ast_peek_is_identifier(FoxyAstParser *parser) {
    /* Mirar si el token actual es seguido inmediatamente por un identificador */
    FoxyLexer lexer_copy = parser->lexer;
    FoxyToken next = f_lexer_next_token(&lexer_copy);
    return next.type == FOX_TOKEN_IDENTIFIER;
}

static void f_ast_consume(FoxyAstParser *parser, FoxyTokenType type, const char *message) {
    if (parser->current_token.type == type) {
        f_ast_advance(parser);
        return;
    }
    f_ast_error_at_current(parser, message);
}

static FoxyAstNode *f_ast_parse_var_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos start_pos = parser->previous_token.pos;
    
    bool is_global = false;
    bool is_static = false;
    bool is_const  = false;

    // Procesar el token disparador
    FoxyTokenType first_type = parser->previous_token.type;
    if (first_type == FOX_TOKEN_KW_GLOBAL) is_global = true;
    else if (first_type == FOX_TOKEN_KW_STATIC) is_static = true;
    else if (first_type == FOX_TOKEN_KW_CONST) is_const = true;

    // Consumir calificadores adicionales en secuencia
    while (parser->current_token.type == FOX_TOKEN_KW_GLOBAL ||
           parser->current_token.type == FOX_TOKEN_KW_STATIC ||
           parser->current_token.type == FOX_TOKEN_KW_CONST) {
        if (parser->current_token.type == FOX_TOKEN_KW_GLOBAL) is_global = true;
        if (parser->current_token.type == FOX_TOKEN_KW_STATIC) is_static = true;
        if (parser->current_token.type == FOX_TOKEN_KW_CONST) is_const = true;
        f_ast_advance(parser);
    }

    FoxyTokenType type_token = FOX_TOKEN_EOF;
    bool is_hybrid = true;

    if (f_ast_is_type_specifier(parser->current_token.type)) {
        type_token = parser->current_token.type;
        is_hybrid = false;
        f_ast_advance(parser);

        // Soporte para corchetes de arreglo en el tipo: int[]
        if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
            f_ast_advance(parser);
            if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                f_ast_advance(parser);
            }
        }
    } else if (parser->current_token.type == FOX_TOKEN_IDENTIFIER && f_ast_peek_is_identifier(parser)) { 
        type_token = FOX_TOKEN_KW_OBJECT;
        is_hybrid = false;
        f_ast_advance(parser);
    }

    if (parser->current_token.type != FOX_TOKEN_IDENTIFIER) {
        f_ast_error_at_current(parser, "Expected variable name.");
        return NULL;
    }

    FoxyToken name_token = parser->current_token;
    f_ast_advance(parser);

    FoxyAstNode *initializer = NULL;
    if (parser->current_token.type == FOX_TOKEN_ASSIGN) {
        f_ast_advance(parser);
        initializer = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_VAR_DECL, start_pos);
    node->as.var_decl.name = name_token;
    node->as.var_decl.type_token = type_token;
    node->as.var_decl.initializer = initializer;
    node->as.var_decl.is_global = is_global;
    node->as.var_decl.is_static = is_static;
    node->as.var_decl.is_const = is_const;
    node->as.var_decl.is_hybrid = is_hybrid;

    return node;
}

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser) {
    if (parser->current_token.type == FOX_TOKEN_KW_INCLUDE) {
        return f_ast_parse_include(parser, NULL, NULL);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_FUNCTION) {
        return f_ast_parse_function_declaration(parser, NULL, false);
    }

    // Si comienza con especificadores de declaración de variable
    if (f_ast_is_var_decl_start(parser->current_token.type)) {
        return f_ast_parse_var_declaration(parser, NULL, NULL);
    }

    // Permitir asignaciones/expresiones directas a nivel top-level/global
    return f_ast_parse_statement(parser);
}

/* ============================================================================
 * IMPLEMENTACIÓN DE REGLAS DE PARSEO DE PRATT Y SENTENCIAS
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
        FoxyAstNode *value = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

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
            f_ast_node_list_append(&node->as.call.args, f_ast_parse_precedence(parser, PREC_ASSIGNMENT));
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true));
    }

    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected ')' after function arguments.");
        f_ast_free_node(node);
        return NULL;
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

        FoxyAstNode *val = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
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
            f_ast_node_list_append(&node->as.array_literal.elements, f_ast_parse_precedence(parser, PREC_ASSIGNMENT));
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
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // Token '['
    
    // Parsear la expresión permitiendo asignaciones compuestas dentro de []
    FoxyAstNode *index_expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
        f_ast_advance(parser); // Consumir ']'
    } else {
        f_ast_error_at_current(parser, "Expected ']' after array index expression.");
        return NULL;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_GET_INDEX, pos);
    node->as.get_index.target = left;
    node->as.get_index.index = index_expr;
    return node;
}

static FoxyAstNode *f_ast_parse_ternary(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxyAstNode *then_branch = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (parser->current_token.type == FOX_TOKEN_COLON) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected ':' in ternary operator.");
        f_ast_free_node(then_branch);
        f_ast_free_node(left);
        return NULL;
    }

    FoxyAstNode *else_branch = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    if (!then_branch || !else_branch) {
        f_ast_free_node(then_branch);
        f_ast_free_node(else_branch);
        f_ast_free_node(left);
        return NULL;
    }

    FoxyAstNode *if_node = f_ast_create_node(FOXY_AST_STMT_IF, left->pos);
    if_node->as.if_stmt.condition = left;
    if_node->as.if_stmt.then_branch = then_branch;
    if_node->as.if_stmt.else_branch = else_branch;

    return if_node;
}

static FoxyAstNode *f_ast_parse_block_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_BLOCK, pos);
    f_ast_node_list_init(&node->as.block_stmt.statements);

    while (parser->current_token.type != FOX_TOKEN_RBRACE && parser->current_token.type != FOX_TOKEN_EOF) {
        FoxyAstNode *stmt = f_ast_parse_declaration(parser);
        if (stmt != NULL) {
            f_ast_node_list_append(&node->as.block_stmt.statements, stmt);
        } else {
            if (parser->panic_mode) {
                f_ast_synchronize(parser);
            } else {
                // Avance de seguridad por si una regla falló silenciosamente sin activar panic_mode
                f_ast_advance(parser);
            }
        }
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected '}' after block.");
    }

    return node;
}

/* ============================================================================
 * PARSEO DE IF / ELSEIF / ELSE
 * ============================================================================ */
static FoxyAstNode *f_ast_parse_if_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    // Solo se avanza si venimos de 'if' o 'elseif' vía dispatch de sentencia
    if (parser->current_token.type == FOX_TOKEN_KW_IF || parser->current_token.type == FOX_TOKEN_KW_ELSEIF) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;
    
    // Parentesis opcionales alrededor de la condición
    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    FoxyAstNode *condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (has_paren) {
        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "Expected ')' after condition.");
            f_ast_free_node(condition);
            return NULL;
        }
    }

    FoxyAstNode *then_branch = f_ast_parse_statement(parser);
    FoxyAstNode *else_branch = NULL;

    if (parser->current_token.type == FOX_TOKEN_KW_ELSEIF) {
        // En lugar de llamar f_ast_parse_if_statement manualmente,
        // permitimos que f_ast_parse_statement lo despache limpiamente
        else_branch = f_ast_parse_statement(parser);
    } else if (parser->current_token.type == FOX_TOKEN_KW_ELSE) {
        f_ast_advance(parser); // Consumir 'else'
        else_branch = f_ast_parse_statement(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_IF, pos);
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

static FoxyAstNode *f_ast_parse_return_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;
    FoxyAstNode *value = NULL;

    if (parser->current_token.type != FOX_TOKEN_SEMICOLON) {
        value = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_RETURN, pos);
    node->as.return_stmt.value = value;
    return node;
}

static FoxyAstNode *f_ast_parse_function_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // Token 'function'

    f_ast_advance(parser);
    
    // Aceptar identificadores normales y metamétodos FOX_TOKEN_METHOD_*
    if (!f_ast_is_valid_func_name(parser->current_token.type)) {
        f_ast_error_at_current(parser, "Expected function or method name.");
        return NULL;
    }

    FoxyToken name = parser->current_token;
    f_ast_advance(parser);

    if (parser->current_token.type != FOX_TOKEN_LPAREN) {
        f_ast_error_at_current(parser, "Expected '(' after function name.");
        f_ast_synchronize(parser);
        return NULL;
    }
    f_ast_advance(parser);

    FoxyAstNodeList params;
    f_ast_node_list_init(&params);

    if (parser->current_token.type != FOX_TOKEN_RPAREN) {
        do {
            if (f_ast_is_type_specifier(parser->current_token.type)) {
                f_ast_advance(parser);
            }

            if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                FoxyAstNode *param = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->current_token.pos);
                param->as.identifier.name = parser->current_token;
                f_ast_node_list_append(&params, param);
                f_ast_advance(parser);

                // Variádico sufijo: valores...
                if (parser->current_token.type == FOX_TOKEN_DOTDOTDOT) {
                    f_ast_advance(parser);
                }
            } else if (parser->current_token.type == FOX_TOKEN_DOTDOTDOT) {
                // Variádico prefijo: ...valores
                f_ast_advance(parser);
                if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                    FoxyAstNode *param = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->current_token.pos);
                    param->as.identifier.name = parser->current_token;
                    f_ast_node_list_append(&params, param);
                    f_ast_advance(parser);
                }
            } else {
                f_ast_error_at_current(parser, "Expected parameter name.");
                f_ast_free_node_list(&params);
                f_ast_synchronize(parser);
                return NULL;
            }
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true));
    }

    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected ')' after parameter list.");
        f_ast_free_node_list(&params);
        f_ast_synchronize(parser);
        return NULL;
    }

    FoxyAstNode *body = NULL;

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    } else if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        body = f_ast_parse_block_statement(parser, NULL, NULL);
        if (!body && parser->had_error) {
            f_ast_free_node_list(&params);
            f_ast_synchronize(parser);
            return NULL;
        }
    } else {
        f_ast_error_at_current(parser, "Expected '{' or ';' after function signature.");
        f_ast_free_node_list(&params);
        f_ast_synchronize(parser);
        return NULL;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_FUNC_DECL, pos);
    node->as.func_decl.name = name;
    node->as.func_decl.params = params;
    node->as.func_decl.body = body;
    return node;
}

static FoxyAstNode *f_ast_parse_while_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left; (void)can_assign;
    FoxySourcePos pos = parser->current_token.pos;
    f_ast_advance(parser); // Consumir 'while'

    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    FoxyAstNode *condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (has_paren) {
        f_ast_consume(parser, FOX_TOKEN_RPAREN, "Expected ')' after while condition.");
    }

    FoxyAstNode *body = f_ast_parse_statement(parser);
    
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_WHILE, pos);
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE ESTRUCTURAS DE CONTROL Y CONTROL DE FLUJO FALTANTES
 * ============================================================================ */

static FoxyAstNode *f_ast_parse_for_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // Token 'for'

    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    // 1. Cláusula de Inicialización
    FoxyAstNode *init = NULL;
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    } else if (f_ast_is_var_decl_start(parser->current_token.type)) {
        init = f_ast_parse_var_declaration(parser, NULL, false);
    } else {
        init = f_ast_parse_expression_statement(parser);
    }

    // 2. Cláusula de Condición
    FoxyAstNode *condition = NULL;
    if (parser->current_token.type != FOX_TOKEN_SEMICOLON) {
        condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    // 3. Cláusula de Incremento
    FoxyAstNode *increment = NULL;
    if (parser->current_token.type != FOX_TOKEN_RPAREN && parser->current_token.type != FOX_TOKEN_LBRACE) {
        increment = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (has_paren) {
        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "Expected ')' after for clauses.");
        }
    }

    FoxyAstNode *body = f_ast_parse_statement(parser);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_FOR, pos);
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = condition;
    node->as.for_stmt.increment = increment;
    node->as.for_stmt.body = body;

    return node;
}

static FoxyAstNode *f_ast_parse_foreach_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // Token 'foreach'

    bool has_outer_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_outer_paren = true;
        f_ast_advance(parser);
    }

    FoxyToken iterator_var = {0};

    // 1. Tupla o desestructuración: foreach ((k, v) : mapa)
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        f_ast_advance(parser);
        
        if (f_ast_is_type_specifier(parser->current_token.type)) {
            f_ast_advance(parser);
        }

        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
            iterator_var = parser->current_token;
            f_ast_advance(parser);
        }

        while (parser->current_token.type == FOX_TOKEN_COMMA) {
            f_ast_advance(parser);
            if (f_ast_is_type_specifier(parser->current_token.type)) {
                f_ast_advance(parser);
            }
            if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                f_ast_advance(parser);
            }
        }

        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        }
    } else {
        // Variable iteradora simple: foreach (number val : arr)
        if (f_ast_is_type_specifier(parser->current_token.type)) {
            f_ast_advance(parser);
        }

        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
            iterator_var = parser->current_token;
            f_ast_advance(parser);
        }
    }

    if (parser->current_token.type == FOX_TOKEN_COLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *iterable = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (has_outer_paren) {
        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        }
    }

    FoxyAstNode *body = f_ast_parse_statement(parser);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_FOREACH, pos);
    node->as.foreach_stmt.iterator_var = iterator_var;
    node->as.foreach_stmt.iterable = iterable;
    node->as.foreach_stmt.body = body;

    return node;
}

static FoxyAstNode *f_ast_parse_switch_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    // Manejo opcional de paréntesis en la condición
    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    FoxyAstNode *condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    if (!condition) return NULL;

    if (has_paren) {
        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "Expected ')' after switch condition.");
            f_ast_free_node(condition);
            return NULL;
        }
    }

    if (parser->current_token.type != FOX_TOKEN_LBRACE) {
        f_ast_error_at_current(parser, "Expected '{' before switch body.");
        f_ast_free_node(condition);
        return NULL;
    }
    f_ast_advance(parser); // Consumir '{'

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_SWITCH, pos);
    node->as.switch_stmt.condition = condition;
    f_ast_node_list_init(&node->as.switch_stmt.cases);

    while (parser->current_token.type != FOX_TOKEN_RBRACE && parser->current_token.type != FOX_TOKEN_EOF) {
        if (parser->current_token.type == FOX_TOKEN_KW_CASE || parser->current_token.type == FOX_TOKEN_KW_DEFAULT) {
            f_ast_advance(parser);
            FoxyAstNode *c = f_ast_parse_case_statement(parser, NULL, false);
            if (c != NULL) {
                f_ast_node_list_append(&node->as.switch_stmt.cases, c);
            }
        } else {
            f_ast_advance(parser);
        }
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected '}' after switch body.");
        f_ast_free_node(node);
        return NULL;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_case_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // Se consumió 'case' o 'default'
    bool is_default = (parser->previous_token.type == FOX_TOKEN_KW_DEFAULT);

    FoxyAstNode *expr = NULL;
    if (!is_default) {
        expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
        if (!expr) return NULL;
    }

    if (parser->current_token.type == FOX_TOKEN_COLON) {
        f_ast_advance(parser); // Consumir ':'
    } else {
        f_ast_error_at_current(parser, "Expected ':' after case/default expression.");
        if (expr) f_ast_free_node(expr);
        return NULL;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_CASE, pos);
    node->as.case_stmt.expr = expr;
    f_ast_node_list_init(&node->as.case_stmt.stmts);

    // Parsear sentencias dentro del caso hasta ver 'case', 'default', '}' o EOF
    while (parser->current_token.type != FOX_TOKEN_KW_CASE &&
       parser->current_token.type != FOX_TOKEN_KW_DEFAULT &&
       parser->current_token.type != FOX_TOKEN_RBRACE &&
       parser->current_token.type != FOX_TOKEN_EOF) {

        FoxyAstNode *stmt = f_ast_parse_statement(parser);
        if (stmt) {
            f_ast_node_list_append(&node->as.case_stmt.stmts, stmt);
        } else {
            f_ast_synchronize(parser);
        }
    }

    return node;
}

static FoxyAstNode *f_ast_parse_break_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    // Consumir ';' opcional si está presente en la sintaxis
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    return f_ast_create_node(FOXY_AST_STMT_BREAK, pos);
}

static FoxyAstNode *f_ast_parse_continue_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    return f_ast_create_node(FOXY_AST_STMT_CONTINUE, pos);
}

static FoxyAstNode *f_ast_parse_goto_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    f_ast_advance(parser); // Consumir 'goto' si no se ha consumido
    if (parser->current_token.type != FOX_TOKEN_IDENTIFIER && 
        parser->current_token.type != FOX_TOKEN_LABEL) {
        f_ast_error_at_current(parser, "Expected label identifier after 'goto'.");
        return NULL;
    }

    FoxyToken label_token = parser->current_token;
    f_ast_advance(parser);

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_GOTO, pos);
    node->as.goto_stmt.label = label_token;
    return node;
}

static FoxyAstNode *f_ast_parse_try_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos; // 'try'

    // Consume 'try' si current_token está sobre el token keyword
    if (parser->current_token.type == FOX_TOKEN_KW_TRY) {
        f_ast_advance(parser);
    }

    // 1. Bloque principal 'try'
    FoxyAstNode *try_block = f_ast_parse_statement(parser);

    FoxyAstNode *try_node = f_ast_create_node(FOXY_AST_STMT_TRY, pos);
    try_node->as.try_stmt.try_block = try_block;
    try_node->as.try_stmt.finally_block = NULL;
    f_ast_node_list_init(&try_node->as.try_stmt.catch_blocks);

    // 2. Bloques 'catch' / 'except'
    while (parser->current_token.type == FOX_TOKEN_KW_CATCH || parser->current_token.type == FOX_TOKEN_KW_EXCEPT) {
        FoxySourcePos catch_pos = parser->current_token.pos;
        f_ast_advance(parser);

        FoxyToken var_name = (FoxyToken){0};
        if (parser->current_token.type == FOX_TOKEN_LPAREN) {
            f_ast_advance(parser);
            
            // Especificador de tipo opcional (ej: char)
            if (f_ast_is_type_specifier(parser->current_token.type)) {
                f_ast_advance(parser);
            }

            if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                var_name = parser->current_token;
                f_ast_advance(parser);
            }

            // Sufijo de arreglo opcional '[]'
            if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
                f_ast_advance(parser);
                if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                    f_ast_advance(parser);
                }
            }

            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "Expected ')' after catch variable.");
                f_ast_advance(parser); // Avance forzado para evitar bucle
            }
        }

        FoxyAstNode *catch_body = f_ast_parse_statement(parser);

        FoxyAstNode *catch_node = f_ast_create_node(FOXY_AST_STMT_CATCH, catch_pos);
        catch_node->as.catch_stmt.var_name = var_name;
        catch_node->as.catch_stmt.body = catch_body;

        f_ast_node_list_append(&try_node->as.try_stmt.catch_blocks, catch_node);
    }

    // 3. Bloque 'finally' / 'final' opcional
    if (parser->current_token.type == FOX_TOKEN_KW_FINAL) {
        f_ast_advance(parser);
        try_node->as.try_stmt.finally_block = f_ast_parse_statement(parser);
    }

    return try_node;
}

static FoxyAstNode *f_ast_parse_unpack(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyToken op = parser->previous_token; // Token '...'
    
    /* Parsea la expresión que sigue a '...' con precedencia de llamada/unaria */
    FoxyAstNode *operand = f_ast_parse_precedence(parser, PREC_UNARY);

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, op.pos);
    node->as.unary.op = op;
    node->as.unary.operand = operand;
    node->as.unary.is_postfix = 0;
    
    return node;
}

static FoxyAstNode *f_ast_parse_export_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left; (void)can_assign;
    // Consumir 'export' y delegar a la declaración interna (enum, class, function, var, etc.)
    return f_ast_parse_declaration(parser);
}

static FoxyAstNode *f_ast_parse_use_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left; (void)can_assign;
    // Consumir identificador y/o subrutinas de use (ej: use Modos.*)
    while (parser->current_token.type != FOX_TOKEN_SEMICOLON && 
           parser->current_token.type != FOX_TOKEN_EOF && 
           parser->current_token.pos.line == parser->previous_token.pos.line) {
        f_ast_advance(parser);
    }
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }
    return NULL; // Se descarta como nodo de metadatos o se emite según la arquitectura
}

static FoxyAstNode *f_ast_parse_enum_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left; (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        f_ast_advance(parser); // Nombre del enum
    }

    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        int depth = 1;
        while (depth > 0 && parser->current_token.type != FOX_TOKEN_EOF) {
            if (parser->current_token.type == FOX_TOKEN_LBRACE) depth++;
            else if (parser->current_token.type == FOX_TOKEN_RBRACE) depth--;
            f_ast_advance(parser);
        }
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_BLOCK, pos);
    f_ast_node_list_init(&node->as.block_stmt.statements);
    return node;
}

static FoxyAstNode *f_ast_parse_class_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left; 
    (void)can_assign;
    // FoxySourcePos pos = parser->previous_token.pos; // Token 'class'

    // Identificador de la clase
    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected class name.");
        return NULL;
    }

    // Cláusula de herencia opcional: 'from BaseFigura'
    if (parser->current_token.type == FOX_TOKEN_KW_FROM) {
        f_ast_advance(parser);
        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "Expected parent class name after 'from'.");
            return NULL;
        }
    }

    // Cuerpo de la clase
    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, false);
    }

    f_ast_error_at_current(parser, "Expected '{' before class body.");
    return NULL;
}

static FoxyAstNode *f_ast_parse_struct_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    // FoxySourcePos pos = parser->previous_token.pos; // Token 'struct'

    // Identificador del struct (ej: Punto)
    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "Expected struct name.");
        return NULL;
    }

    // Cuerpo del struct (definición de campos/miembros)
    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, false);
    }

    f_ast_error_at_current(parser, "Expected '{' before struct body.");
    return NULL;
}