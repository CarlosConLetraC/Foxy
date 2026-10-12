#include "f_ast.h"
// #include "f_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    "FOXY_AST_STMT_INCLUDE",
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
    "FOXY_AST_STMT_CATCH",
    "FOXY_AST_STMT_LAMBDA"
};
#endif

// TODO: separar parseador de ast en f_parser.h y f_parser.c

/* Forward Declarations internas */
static void f_ast_advance(FoxyAstParser *parser);
static void f_ast_synchronize(FoxyAstParser *parser);
static void f_ast_consume(FoxyAstParser *parser, FoxyTokenType type, const char *message);
static void f_ast_error_at_current(FoxyAstParser *parser, const char *message);
static bool f_ast_peek_is_identifier(FoxyAstParser *parser);
static bool f_ast_peek_is_type_specifier(FoxyAstParser *parser);
static bool f_ast_is_cast_expression_pattern(FoxyAstParser *parser);
static bool f_ast_is_lambda_type_signature(FoxyAstParser *parser);
static bool f_ast_is_type_signature(FoxyAstParser *parser);

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_statement(FoxyAstParser *parser);
static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence);
static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type);
static FoxyAstNode *f_ast_parse_expression_statement(FoxyAstParser *parser);

/* Reglas de Parseo (FoxyParseFn: parser, left, can_assign) */
static FoxyAstNode *f_ast_parse_include(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_var_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
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
static FoxyAstNode *f_ast_parse_dict_or_array(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
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
static FoxyAstNode *f_ast_parse_lambda(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);
static FoxyAstNode *f_ast_parse_type_prefix(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign);

static const FoxyParseRule rules[] = {
    [FOX_TOKEN_EOF]              = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_ERROR]            = { NULL,                NULL,              PREC_NONE },

    /* Identificadores y Literales */
    [FOX_TOKEN_IDENTIFIER]          = { f_ast_parse_variable,NULL,              PREC_NONE },
    [FOX_TOKEN_DATATYPE_IDENTIFIER] = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_LABEL]               = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_INT_LITERAL]         = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_UINT_LITERAL]        = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LONG_LITERAL]        = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_ULONG_LITERAL]       = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LLONG_LITERAL]       = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_ULLONG_LITERAL]      = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_FLOAT_LITERAL]       = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_DOUBLE_LITERAL]      = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_LDOUBLE_LITERAL]     = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_NUMBER_LITERAL]      = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_CHAR_LITERAL]        = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_STRING_LITERAL]      = { f_ast_parse_literal, NULL,              PREC_NONE },

    /*
     * Palabras Clave / Especificadores / Tipos 
     * NOTA: Los tipos NO deben apuntar a f_ast_parse_unary en prefix. 
     * Las declaraciones se procesan en f_ast_parse_declaration/var_declaration.
     */
    [FOX_TOKEN_KW_GLOBAL]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_STATIC]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CONST]         = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_NULL]          = { f_ast_parse_literal, NULL,              PREC_NONE },
    [FOX_TOKEN_KW_BOOL]          = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_CHAR]          = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_UCHAR]         = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_SHORT]         = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_USHORT]        = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_INT]           = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_UINT]          = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_LONG]          = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_ULONG]         = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_LLONG]         = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_ULLONG]        = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_FLOAT]         = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_DOUBLE]        = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_LDOUBLE]       = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_NUMBER]        = { f_ast_parse_type_prefix, NULL,          PREC_NONE },
    [FOX_TOKEN_KW_DICT]          = { f_ast_parse_type_prefix, NULL,          PREC_NONE },

    /* Control de Flujo */
    [FOX_TOKEN_KW_IF]            = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ELSEIF]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_ELSE]          = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_WHILE]         = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FOR]           = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FOREACH]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_SWITCH]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CASE]          = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_DEFAULT]       = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_RETURN]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_BREAK]         = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CONTINUE]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_GOTO]          = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_TRY]           = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_CATCH]         = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_EXCEPT]        = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FINAL]         = { NULL,                NULL,              PREC_NONE },

    /* POO y Estructuras */
    [FOX_TOKEN_KW_CLASS]         = { f_ast_parse_class_declaration,  NULL,   PREC_NONE },
    [FOX_TOKEN_KW_STRUCT]        = { f_ast_parse_struct_declaration, NULL,   PREC_NONE },
    [FOX_TOKEN_KW_ENUM]          = { f_ast_parse_enum_declaration,NULL,      PREC_NONE },
    [FOX_TOKEN_KW_FROM]          = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_FUNCTION]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_KW_OVERRULE]      = { NULL,                NULL,              PREC_NONE },
    [FOX_TOKEN_KW_INCLUDE]       = { NULL,                NULL,              PREC_NONE },
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
    [FOX_TOKEN_METHOD_NEW]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_CAST]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_TOSTRING]  = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_ADD]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_SUB]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_MUL]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_DIV]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_POW]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_MOD]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_CONCAT]    = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_UNM]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_NOT]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_EQ]        = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_NEQ]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_LT]        = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_GT]        = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_LE]        = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_GE]        = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_BAND]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_BOR]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_BNOT]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_BXOR]      = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_LSHIFT]    = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_RSHIFT]    = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_FOREACH]   = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_CLOSED]    = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_LEN]       = { f_ast_parse_function_declaration, NULL, PREC_NONE },
    [FOX_TOKEN_METHOD_UNPACK]    = { f_ast_parse_function_declaration, NULL, PREC_NONE },

    /* Operadores Aritméticos, Lógicos, Bitwise y Asignaciones */
    [FOX_TOKEN_PLUS]             = { NULL,               f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_MINUS]            = { f_ast_parse_unary,  f_ast_parse_binary, PREC_TERM },
    [FOX_TOKEN_STAR]             = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_SLASH]            = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_PERCENT]          = { NULL,               f_ast_parse_binary, PREC_FACTOR },
    [FOX_TOKEN_POWER]            = { NULL,               f_ast_parse_binary, PREC_POWER },
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
    [FOX_TOKEN_FAT_ARROW]        = { NULL,               f_ast_parse_lambda, PREC_LAMBDA },
    [FOX_TOKEN_PTR_ARROW]        = { NULL,               f_ast_parse_member, PREC_CALL },
    [FOX_TOKEN_LPAREN]           = { f_ast_parse_grouping,f_ast_parse_call,  PREC_CALL },
    [FOX_TOKEN_RPAREN]           = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_LBRACE]           = { f_ast_parse_dict_or_array, NULL,        PREC_NONE },
    [FOX_TOKEN_RBRACE]           = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_LBRACKET]         = { f_ast_parse_dict,   f_ast_parse_index,  PREC_CALL },
    [FOX_TOKEN_RBRACKET]         = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_SEMICOLON]        = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_COLON]            = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_COMMA]            = { NULL,               NULL,               PREC_NONE },
    [FOX_TOKEN_DOT]              = { NULL,               f_ast_parse_member, PREC_CALL },
    [FOX_TOKEN_DOTDOT]           = { NULL,               f_ast_parse_binary, PREC_COMPARISON },
    [FOX_TOKEN_DOTDOTDOT]        = { f_ast_parse_unpack, NULL,               PREC_UNARY },
    [FOX_TOKEN_QUESTION]         = { NULL,               f_ast_parse_ternary,PREC_ASSIGNMENT }
};

static inline void f_ast_debugger(FoxyAstParser *parser) {
    const char *line_start = "";
    int line_len = 0;

    if (parser && parser->lexer.source) {
        const char *ref = parser->current_token.start ? parser->current_token.start : parser->lexer.cursor;
        
        if (ref && ref >= parser->lexer.source) {
            const char *start = ref;
            while (start > parser->lexer.source && *(start - 1) != '\n') {
                start--;
            }

            const char *end = ref;
            while (*end != '\0' && *end != '\n' && *end != '\r') {
                end++;
            }

            line_start = start;
            line_len = (int)(end - start);
        }
    }

    printf("{\n"
           "\tline [%u]: %.*s\n"
           "\tparser->current_token.type: %s<%d>\n"
           "\tparser->previous_token.type: %s<%d>\n"
           "}\n",
           parser->current_token.pos.line,
           line_len, line_start,
           f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type,
           f_lexer_token_type_to_string(parser->previous_token.type), parser->previous_token.type
    );
}

static inline bool f_ast_is_valid_func_name(FoxyAstParser *parser) {
    FoxyTokenType type = parser->current_token.type;
    return type == FOX_TOKEN_IDENTIFIER || (type >= FOX_TOKEN_METHOD_NEW && type <= FOX_TOKEN_METHOD_UNPACK);
}

static inline bool f_ast_peek_is_type_specifier(FoxyAstParser *parser) {
    FoxyLexer lexer_copy = parser->lexer;
    FoxyToken next = f_lexer_next_token(&lexer_copy);
    return f_ast_is_type_specifier(next.type);
}

static inline bool f_ast_is_var_decl_start(FoxyAstParser *parser) {
    FoxyToken tok = parser->current_token;
    if (tok.type_category == FOXY_TOKEN_CAT_KEYWORD && 
        (tok.type == FOX_TOKEN_KW_GLOBAL ||
         tok.type == FOX_TOKEN_KW_STATIC ||
         tok.type == FOX_TOKEN_KW_CONST)) {
        return true;
    }
    
    // Verificación limpia y directa de cualquier tipo soportado por el lenguaje
    if (f_ast_is_type_specifier(tok.type)) {
        return true;
    }
    
    if (tok.type == FOX_TOKEN_IDENTIFIER) {
        return f_ast_peek_is_identifier(parser);
    }
    return false;
}

static inline bool f_ast_is_cast_expression_pattern(FoxyAstParser *parser) {
    // 1. Debe iniciar con un especificador de tipo válido
    if (!f_ast_is_type_specifier(parser->current_token.type)) {
        return false;
    }

    FoxyLexer lexer_copy = parser->lexer;
    FoxyToken token = f_lexer_next_token(&lexer_copy);

    // Verificar si incluye corchetes de arreglo opcionales vacíos []
    if (token.type == FOX_TOKEN_LBRACKET) {
        token = f_lexer_next_token(&lexer_copy);
        if (token.type == FOX_TOKEN_RBRACKET) {
            token = f_lexer_next_token(&lexer_copy);
        } else {
            return false;
        }
    }

    // La secuencia debe cerrar inmediatamente con ')'
    return (token.type == FOX_TOKEN_RPAREN);
}

static inline bool f_ast_is_lambda_type_signature(FoxyAstParser *parser) {
    FoxyLexer lexer_copy = parser->lexer;
    int paren_depth = 1;
    bool has_types = false;
    bool has_identifiers = false;

    while (paren_depth > 0) {
        FoxyToken tok = f_lexer_next_token(&lexer_copy);
        if (tok.type == FOX_TOKEN_EOF) break;
        
        if (tok.type == FOX_TOKEN_LPAREN) {
            paren_depth++;
        } else if (tok.type == FOX_TOKEN_RPAREN) {
            paren_depth--;
            if (paren_depth == 0) break;
        } else if (paren_depth == 1) {
            if (f_ast_is_type_specifier(tok.type)) {
                has_types = true;
            } else if (tok.type == FOX_TOKEN_IDENTIFIER) {
                has_identifiers = true;
            }
        }
    }

    // Es firma de tipos si contiene tipos pero carece de identificadores de variables internos
    return has_types && !has_identifiers;
}

// Función auxiliar para verificar si un token o secuencia representa una firma de tipo tipo (int, int) o (int)
static inline bool f_ast_is_type_signature(FoxyAstParser *parser) {
    FoxyLexer lexer_copy = parser->lexer;
    FoxyToken tok;

    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        tok = f_lexer_next_token(&lexer_copy);
    } else if (parser->previous_token.type == FOX_TOKEN_LPAREN) {
        tok = parser->current_token;
    } else {
        return false;
    }

    bool has_types = false;

    while (tok.type != FOX_TOKEN_RPAREN && tok.type != FOX_TOKEN_EOF) {
        if (!f_ast_is_type_specifier(tok.type)) {
            return false;
        }
        has_types = true;
        
        tok = f_lexer_next_token(&lexer_copy);
        if (tok.type == FOX_TOKEN_COMMA) {
            tok = f_lexer_next_token(&lexer_copy);
        } else if (tok.type != FOX_TOKEN_RPAREN) {
            return false;
        }
    }
    
    if (!has_types || tok.type != FOX_TOKEN_RPAREN) {
        return false;
    }
    
    // Verificar que después del paréntesis de cierre exista obligatoriamente un identificador (el nombre de la variable)
    FoxyToken next_tok = f_lexer_next_token(&lexer_copy);
    return next_tok.type == FOX_TOKEN_IDENTIFIER;
}

// función auxiliar para verificar si un token funciona como Tipo Válido (Primitivo, Estructura, Clase, Enum o Identificador que actúe como Tipo)
static inline bool f_ast_is_any_type(FoxyAstParser *parser, FoxyToken token) {
    (void)parser;
    if (f_ast_is_type_specifier(token.type)) {
        return true;
    }
    // Si es un identificador y el siguiente token es también un identificador, 
    // asumimos que el primero es una clase/estructura o tipo personalizado (Ej: Node currentNode)
    if (token.type == FOX_TOKEN_IDENTIFIER) {
        return true; 
    }
    return false;
}

static const FoxyParseRule *f_ast_get_rule(FoxyTokenType type) {
    return &rules[type];
}

static FoxyAstNode *f_ast_parse_expression_statement(FoxyAstParser *parser) {
    FoxySourcePos pos = parser->current_token.pos;
    FoxyAstNode *expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (expr == NULL) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_expression_statement] Expected expression.");
        f_ast_advance(parser); // Fuerza el avance para evitar bucle infinito
        return NULL;
    }

    // Consumir ';' opcional
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    // Si la expresión ya es un nodo de sentencia, se retorna directamente
    if (FOXY_AST_IS_STMT_KIND(expr->kind)) {
        return expr;
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

static FoxyGroupKind f_ast_classify_group(FoxyAstParser *parser) {
    // 0. Prioridad absoluta: Si es una firma de tipo de variable (ej: (int) name o (int, int) name)
    if (f_ast_is_type_signature(parser)) {
        return FOXY_GROUP_LAMBDA_PARAMS; // O un tratamiento de firma de tipo
    }

    FoxyToken first = parser->current_token; 
    FoxyLexer lexer_copy = parser->lexer;
    FoxyToken second = f_lexer_next_token(&lexer_copy);

    // 1. Verificar si es un casteo simple: (tipo) o (tipo[]) seguido de valor o fin de grupo
    if (f_ast_is_type_specifier(first.type)) {
        if (second.type == FOX_TOKEN_RPAREN) {
            return FOXY_GROUP_CAST;
        }
        if (second.type == FOX_TOKEN_LBRACKET) {
            FoxyToken third = f_lexer_next_token(&lexer_copy);
            FoxyToken fourth = f_lexer_next_token(&lexer_copy);
            if (third.type == FOX_TOKEN_RBRACKET && fourth.type == FOX_TOKEN_RPAREN) {
                FoxyToken fifth = f_lexer_next_token(&lexer_copy);
                if (fifth.type == FOX_TOKEN_LBRACE) {
                    return FOXY_GROUP_ARRAY_CAST;
                }
                return FOXY_GROUP_CAST;
            }
        }
    }

    // 2. Escanear el interior completo del grupo
    FoxyLexer scan_lexer = parser->lexer;
    int depth = 1;
    bool has_type = f_ast_is_type_specifier(first.type);
    bool has_identifier = (first.type == FOX_TOKEN_IDENTIFIER);
    bool has_comma = false;
    bool has_comparison = false;

    FoxyToken curr = first;
    while (depth > 0 && curr.type != FOX_TOKEN_EOF) {
        if (curr.type == FOX_TOKEN_LPAREN) {
            depth++;
        } else if (curr.type == FOX_TOKEN_RPAREN) {
            depth--;
            if (depth == 0) break;
        } else {
            if (depth == 1) {
                if (f_ast_is_type_specifier(curr.type)) has_type = true;
                if (curr.type == FOX_TOKEN_IDENTIFIER) has_identifier = true;
                if (curr.type == FOX_TOKEN_COMMA) has_comma = true;
                if (curr.type == FOX_TOKEN_EQ  || curr.type == FOX_TOKEN_NEQ ||
                    curr.type == FOX_TOKEN_LT  || curr.type == FOX_TOKEN_GT  ||
                    curr.type == FOX_TOKEN_LE  || curr.type == FOX_TOKEN_GE) {
                    has_comparison = true;
                }
            }
        }
        curr = f_lexer_next_token(&scan_lexer);
    }

    FoxyToken after_paren = f_lexer_next_token(&scan_lexer);
    bool followed_by_fat_arrow = (after_paren.type == FOX_TOKEN_FAT_ARROW);

    if (followed_by_fat_arrow || (has_type && has_identifier)) {
        return FOXY_GROUP_LAMBDA_PARAMS;
    }

    if (has_comma && !has_type) {
        return FOXY_GROUP_TUPLE_BINDING;
    }

    if (has_comparison) {
        return FOXY_GROUP_PURE;
    }

    return FOXY_GROUP_PURE;
}

static FoxyAstNode *f_ast_parse_grouping(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    FoxySourcePos pos = parser->previous_token.pos;
    FoxyGroupKind group_kind = f_ast_classify_group(parser);

    switch (group_kind) {
        case FOXY_GROUP_PURE: {
            FoxyAstNode *expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "Expected ')' after grouped expression.");
            }
            if (expr != NULL && expr->kind == FOXY_AST_EXPR_ASSIGN) {
                expr->as.assign.is_grouped = 1;
            }
            return expr;
        }

        case FOXY_GROUP_CAST: {
            FoxyToken type_token_var = parser->current_token;
            f_ast_advance(parser); // Consumir tipo

            bool is_array_cast = false;
            if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
                f_ast_advance(parser);
                if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                    f_ast_advance(parser);
                    is_array_cast = true;
                }
            }

            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "Expected ')' after cast type.");
                return NULL;
            }

            if (is_array_cast && parser->current_token.type == FOX_TOKEN_LBRACE) {
                f_ast_advance(parser);
                FoxyAstNode *array_node = f_ast_create_node(FOXY_AST_EXPR_ARRAY_LITERAL, pos);
                array_node->as.array_literal.token_type = type_token_var.type;
                f_ast_node_list_init(&array_node->as.array_literal.elements);

                if (parser->current_token.type != FOX_TOKEN_RBRACE) {
                    do {
                        if (parser->current_token.type == FOX_TOKEN_RBRACE || parser->current_token.type == FOX_TOKEN_EOF) break;
                        FoxyAstNode *elem = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
                        if (elem) f_ast_node_list_append(&array_node->as.array_literal.elements, elem);
                        if (parser->current_token.type == FOX_TOKEN_COMMA) f_ast_advance(parser);
                        else break;
                    } while (parser->current_token.type != FOX_TOKEN_RBRACE);
                }
                if (parser->current_token.type == FOX_TOKEN_RBRACE) f_ast_advance(parser);
                return array_node;
            }

            FoxyAstNode *operand = f_ast_parse_precedence(parser, PREC_UNARY);
            if (!operand) return NULL;

            FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, type_token_var.pos);
            node->as.unary.op = type_token_var;
            node->as.unary.operand = operand;
            node->as.unary.is_postfix = 0;
            return node;
        }

        case FOXY_GROUP_ARRAY_CAST: {
            FoxyToken type_token_var = parser->current_token;
            f_ast_advance(parser); // Consumir tipo
            f_ast_advance(parser); // '['
            f_ast_advance(parser); // ']'
            f_ast_advance(parser); // ')'
            f_ast_advance(parser); // '{'

            FoxyAstNode *array_node = f_ast_create_node(FOXY_AST_EXPR_ARRAY_LITERAL, pos);
            array_node->as.array_literal.token_type = type_token_var.type;
            f_ast_node_list_init(&array_node->as.array_literal.elements);

            if (parser->current_token.type != FOX_TOKEN_RBRACE) {
                do {
                    if (parser->current_token.type == FOX_TOKEN_RBRACE || parser->current_token.type == FOX_TOKEN_EOF) break;
                    FoxyAstNode *elem = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
                    if (elem) f_ast_node_list_append(&array_node->as.array_literal.elements, elem);
                    if (parser->current_token.type == FOX_TOKEN_COMMA) f_ast_advance(parser);
                    else break;
                } while (parser->current_token.type != FOX_TOKEN_RBRACE);
            }
            if (parser->current_token.type == FOX_TOKEN_RBRACE) f_ast_advance(parser);
            return array_node;
        }

        case FOXY_GROUP_TUPLE_BINDING: {
            FoxyAstNode *expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

            while (parser->current_token.type == FOX_TOKEN_COMMA) {
                FoxyToken comma_op = parser->current_token;
                f_ast_advance(parser);
                FoxyAstNode *next_expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
                if (expr && next_expr) {
                    expr = f_ast_create_binary_node(comma_op, expr, next_expr);
                }
            }

            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "Expected ')' after tuple binding expression.");
                if (expr) f_ast_free_node(expr);
                return NULL;
            }

            return expr;
        }

        case FOXY_GROUP_LAMBDA_PARAMS: {
            FoxyAstNode *block = f_ast_create_node(FOXY_AST_STMT_BLOCK, pos);
            f_ast_node_list_init(&block->as.block_stmt.statements);

            if (parser->current_token.type != FOX_TOKEN_RPAREN) {
                do {
                    if (parser->current_token.type == FOX_TOKEN_RPAREN || parser->current_token.type == FOX_TOKEN_EOF) break;
                    
                    if (f_ast_is_type_specifier(parser->current_token.type)) {
                        FoxyTokenType t_type = parser->current_token.type;
                        f_ast_advance(parser);
                        
                        FoxySourcePos param_pos = parser->previous_token.pos;
                        FoxyAstNode *param = f_ast_create_node(FOXY_AST_STMT_VAR_DECL, param_pos);
                        param->as.var_decl.type_token = t_type;

                        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                            param->as.var_decl.name = parser->current_token;
                            f_ast_advance(parser);
                        }
                        f_ast_node_list_append(&block->as.block_stmt.statements, param);
                    } else if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                        FoxyToken id_token = parser->current_token;
                        f_ast_advance(parser);

                        FoxyAstNode *param = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, id_token.pos);
                        param->as.identifier.name = id_token;
                        f_ast_node_list_append(&block->as.block_stmt.statements, param);
                    } else {
                        break;
                    }

                    if (parser->current_token.type == FOX_TOKEN_COMMA) {
                        f_ast_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current_token.type != FOX_TOKEN_RPAREN);
            }

            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "Expected ')' after lambda parameter list.");
                f_ast_free_node(block);
                return NULL;
            }

            return block;
        }
    }

    return NULL;
}

static FoxyAstNode *f_ast_parse_variable(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    // =========================================================================
    // DEPURADOR PERSONALIZADO EN LÍNEA (VARIABLE / IDENTIFIER)
    // =========================================================================
    // printf("{\n"
    //        "\t[DEBUG VARIABLE] line [%u]\n"
    //        "\t\tprev_token: %s<%d> | curr_token: %s<%d>\n"
    //        "}\n",
    //        parser->current_token.pos.line,
    //        f_lexer_token_type_to_string(parser->previous_token.type), parser->previous_token.type,
    //        f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type
    // );

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
        fprintf(stderr, "[Foxy AST Panic] Memory allocation failed in f_ast_strdup.\n");
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

    if (f_ast_kind_is_leaf(node->kind)) {
        free(node);
        return;
    }

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
    f_ast_value_free(&node->as.literal.value);
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
    f_ast_free_node(node->as.var_decl.array_size);
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

L_FREE_FOXY_AST_STMT_LAMBDA:
    f_ast_free_node_list(&node->as.lambda_stmt.params);
    f_ast_free_node(node->as.lambda_stmt.body);
    goto L_FREE_END;

L_FREE_END:
#else
    switch (node->kind) {
        case FOXY_AST_EXPR_LITERAL:
            f_ast_value_free(&node->as.literal.value);
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

        case FOXY_AST_EXPR_ARRAY_LITERAL:
            f_ast_free_node_list(&node->as.array_literal.elements);
            break;

        case FOXY_AST_EXPR_DICT_LITERAL:
            f_ast_free_node_list(&node->as.dict_literal.entries);
            break;

        case FOXY_AST_STMT_EXPR:
            f_ast_free_node(node->as.expr_stmt);
            break;

        case FOXY_AST_STMT_VAR_DECL:
            f_ast_free_node(node->as.var_decl.initializer);
            f_ast_free_node(node->as.var_decl.array_size);
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

        case FOXY_AST_STMT_LAMBDA:
            f_ast_free_node_list(&node->as.lambda_stmt.params);
            f_ast_free_node(node->as.lambda_stmt.body);
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

static void print_trimmed_float(long double val, bool has_suffix, char suffix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.10Lf", val);
    
    size_t len = strlen(buf);
    while (len > 1 && buf[len - 1] == '0' && buf[len - 2] != '.') {
        buf[--len] = '\0';
    }
    
    printf("%s%c", buf, has_suffix && suffix != '\0' ? tolower(suffix) : '\0');
}

static void f_ast_value_print(FoxyAstValue val) {
    switch (val.type) {
        case FOXY_AST_VAL_NULL:   printf("null"); break;
        case FOXY_AST_VAL_BOOL:   printf("%s", val.as.f_bool ? "true" : "false"); break;
        case FOXY_AST_VAL_INT:    printf("%d", val.as.f_int); break;
        case FOXY_AST_VAL_UINT:   printf("%u", val.as.f_uint); break;
        case FOXY_AST_VAL_LONG:   printf("%ld", val.as.f_long); break;
        case FOXY_AST_VAL_ULONG:  printf("%lu", val.as.f_ulong); break;
        case FOXY_AST_VAL_LLONG:  printf("%lld", val.as.f_llong); break;
        case FOXY_AST_VAL_ULLONG: printf("%llu", val.as.f_ullong); break;
        case FOXY_AST_VAL_FLOAT:  
            print_trimmed_float((long double)val.as.f_float, val.has_explicit_suffix, val.explicit_suffix != '\0' ? val.explicit_suffix : 'f'); 
            break;
        case FOXY_AST_VAL_DOUBLE: 
            print_trimmed_float((long double)val.as.f_double, val.has_explicit_suffix, val.explicit_suffix != '\0' ? val.explicit_suffix : 'd'); 
            break;
        case FOXY_AST_VAL_LDOUBLE:
            print_trimmed_float(val.as.f_ldouble, val.has_explicit_suffix, val.explicit_suffix != '\0' ? val.explicit_suffix : 'l'); 
            break;
        case FOXY_AST_VAL_CHAR:   
            switch (val.as.f_char) {
                case '\n': printf("'\\n'"); break;
                case '\r': printf("'\\r'"); break;
                case '\t': printf("'\\t'"); break;
                case '\b': printf("'\\b'"); break;
                case '\a': printf("'\\a'"); break;
                case '\f': printf("'\\f'"); break;
                case '\v': printf("'\\v'"); break;
                case '\\': printf("'\\\\'"); break;
                case '\'': printf("'\\''"); break;
                case '\0': printf("'\\0'"); break;
                default:
                    if ((unsigned char)val.as.f_char < 32 || (unsigned char)val.as.f_char > 126) {
                        printf("'\\x%02X'", (unsigned char)val.as.f_char);
                    } else {
                        printf("'%c'", val.as.f_char);
                    }
                    break;
            }
            break;
        case FOXY_AST_VAL_STRING: printf("\"%s\"", val.as.f_string); break;
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
            f_ast_value_print(node->as.literal.value);
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
            printf("Var: %.*s (type_token: %d%s%s%s%s)\n", 
                   node->as.var_decl.name.length, 
                   node->as.var_decl.name.start, 
                   node->as.var_decl.type_token,
                   node->as.var_decl.is_global ? ", global" : "",
                   node->as.var_decl.is_static ? ", static" : "",
                   node->as.var_decl.is_const ? ", const" : "",
                   node->as.var_decl.is_array ? ", array" : "");
            
            if (node->as.var_decl.array_size) {
                print_indent(indent + 2);
                printf("Array Size Dimension:\n");
                f_ast_print(node->as.var_decl.array_size, indent + 2);
            }
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

        case FOXY_AST_STMT_LAMBDA:
            print_node_list(&node->as.lambda_stmt.params, indent + 1);
            if (node->as.lambda_stmt.body) {
                f_ast_print(node->as.lambda_stmt.body, indent + 1);
            }
            break;

        default:
            break;
    }
}

/* ============================================================================
 * CONVERSIÓN DE TOKENS A VALORES
 * ============================================================================ */

FoxyAstValue f_ast_value_from_token(const FoxyToken *token) {
    if (!token) return f_ast_new_null();

    switch (token->type) {
        case FOX_TOKEN_KW_TRUE:
            return f_ast_new_bool(true);
        case FOX_TOKEN_KW_FALSE:
            return f_ast_new_bool(false);
        case FOX_TOKEN_KW_NULL:
            return f_ast_new_null();
        case FOX_TOKEN_INT_LITERAL:
            return f_ast_new_int((int)strtol(token->start, NULL, 10));
        case FOX_TOKEN_UINT_LITERAL:
            return f_ast_new_uint((unsigned int)strtoul(token->start, NULL, 10));
        case FOX_TOKEN_LONG_LITERAL:
            return f_ast_new_long(strtol(token->start, NULL, 10));
        case FOX_TOKEN_ULONG_LITERAL:
            return f_ast_new_ulong(strtoul(token->start, NULL, 10));
        case FOX_TOKEN_LLONG_LITERAL:
            return f_ast_new_llong(strtoll(token->start, NULL, 10));
        case FOX_TOKEN_ULLONG_LITERAL:
            return f_ast_new_ullong(strtoull(token->start, NULL, 10));
        case FOX_TOKEN_FLOAT_LITERAL:
            return f_ast_new_float((float)strtod(token->start, NULL));
        case FOX_TOKEN_DOUBLE_LITERAL:
            return f_ast_new_double(strtod(token->start, NULL));
        case FOX_TOKEN_LDOUBLE_LITERAL:
            return f_ast_new_ldouble(strtold(token->start, NULL));
        case FOX_TOKEN_NUMBER_LITERAL:
            return f_ast_new_double(strtod(token->start, NULL));
        case FOX_TOKEN_CHAR_LITERAL: {
            if (token->length >= 3) {
                const char *p = token->start + 1; // Omitir comilla inicial
                // Lógica de decodificación para el char
                char val = 0;
                if (*p == '\\') {
                    p++;
                    char esc = *p++;
                    switch (esc) {
                        case 'n': val = '\n'; break;
                        case 'r': val = '\r'; break;
                        case 't': val = '\t'; break;
                        case 'b': val = '\b'; break;
                        case 'a': val = '\a'; break;
                        case 'f': val = '\f'; break;
                        case 'v': val = '\v'; break;
                        case '\\': val = '\\'; break;
                        case '\'': val = '\''; break;
                        case '\"': val = '\"'; break;
                        case '?': val = '\?'; break;
                        case '0': val = '\0'; break;
                        case 'x': {
                            int hex = 0;
                            for(int i=0; i<2 && isxdigit((unsigned char)*p); i++) {
                                int d = *p++;
                                hex = hex * 16 + (isdigit(d) ? d - '0' : (tolower(d) - 'a' + 10));
                            }
                            val = (char)hex;
                            break;
                        }
                        default:
                            if (esc >= '0' && esc <= '7') {
                                int oct = esc - '0';
                                for(int i=0; i<2 && *p >= '0' && *p <= '7'; i++) {
                                    oct = oct * 8 + (*p++ - '0');
                                }
                                val = (char)oct;
                            } else {
                                val = esc;
                            }
                            break;
                    }
                } else {
                    val = *p;
                }
                return f_ast_new_char(val);
            }
            return f_ast_new_char(0);
        }
        case FOX_TOKEN_STRING_LITERAL: {
            size_t len = token->length >= 2 ? token->length - 2 : 0;
            char *str_content = f_ast_strdup(token->start + 1, len);
            return f_ast_new_string(str_content);
        }
        default:
            return f_ast_new_null();
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
        fprintf(stderr, "[Foxy Parser Error] Line %u: [DEBUG: f_ast_advance] %.*s\n", 
                parser->current_token.pos.line, 
                parser->current_token.length, 
                parser->current_token.start);
    }
    // printf("[DEBUG: AST ADVANCE] ");
    // f_ast_debugger(parser);
}

static void f_ast_synchronize(FoxyAstParser *parser) {
    parser->panic_mode = false;

    if (parser->current_token.type == FOX_TOKEN_EOF) {
        return;
    }

    // printf("f_ast_synchronize[manual advance] := "); f_ast_debugger(parser);
    f_ast_advance(parser);

    while (parser->current_token.type != FOX_TOKEN_EOF) {
        if (f_ast_is_stmt_start(parser->current_token.type) || f_ast_is_var_decl_start(parser)) {
            // printf("f_ast_synchronize[automatic advance[2]] := "); f_ast_debugger(parser);
            return;
        }

        f_ast_advance(parser);
    }
}

static void f_ast_error_at_current(FoxyAstParser *parser, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    printf("[DEBUG: AST ADVANCE [ERROR]] >>> "); f_ast_debugger(parser);
    fprintf(stderr, "[Foxy Parser Error] Line %u: %s\n\n", parser->current_token.pos.line, message);
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

// printf("[DEBUG: AST PREC] Entering parse_precedence with target_prec=%d | Current Token: ", precedence);
// f_ast_debugger(parser);
static FoxyAstNode *f_ast_parse_precedence(FoxyAstParser *parser, FoxyPrecedence precedence) {
    if (parser->current_token.type == FOX_TOKEN_EOF || parser->previous_token.type == FOX_TOKEN_EOF) {
        return NULL;
    }

    f_ast_advance(parser);

    FoxyParseFn prefix_rule = f_ast_get_rule(parser->previous_token.type)->prefix;
    if (prefix_rule == NULL) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_precedence] Expected expression.");
        return NULL;
    }

    FoxyAstNode *node = prefix_rule(parser, NULL, precedence <= PREC_ASSIGNMENT);

    while (precedence <= f_ast_get_rule(parser->current_token.type)->precedence) {
        const FoxyParseRule *rule = f_ast_get_rule(parser->current_token.type);
        FoxyParseFn infix_rule = rule->infix;
        if (infix_rule == NULL) {
            break;
        }

        f_ast_advance(parser);
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

    // Ajuste de precedencia para asociatividad (manejo especial si es operador de potencia u otro)
    FoxyPrecedence next_prec = (FoxyPrecedence)(rule->precedence + 1);
    
    FoxyAstNode *right = f_ast_parse_precedence(parser, next_prec);
    if (!right) {
        f_ast_free_node(left);
        return NULL;
    }

    return f_ast_create_binary_node(op, left, right);
}

static FoxyAstNode *f_ast_parse_statement(FoxyAstParser *parser) {
    if (parser->current_token.type == FOX_TOKEN_EOF) {
        return NULL;
    }

    // printf("[AST STMT] Processing statement at -> ");
    // f_ast_debugger(parser);
    
    if (parser->panic_mode) f_ast_synchronize(parser);

    if (parser->current_token.type == FOX_TOKEN_LABEL) {
        FoxyToken label_token = parser->current_token;
        f_ast_advance(parser);

        FoxyAstNode *label_node = f_ast_create_node(FOXY_AST_STMT_LABEL, label_token.pos);
        label_node->as.goto_stmt.label = label_token;

        // Si hay una sentencia justo después de la etiqueta en la misma línea, adjuntarla o procesarla
        if (parser->current_token.type != FOX_TOKEN_SEMICOLON && 
            parser->current_token.type != FOX_TOKEN_RBRACE &&
            parser->current_token.type != FOX_TOKEN_EOF) {
            // Opcional: puedes parsear la sentencia adyacente si la gramática lo permite
            FoxyAstNode *inline_stmt = f_ast_parse_statement(parser);
            (void)inline_stmt;
            // Aquí podrías envolverlos en un bloque si el AST lo requiere
        }
        return label_node;
    }

    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, false);
    }

    // Detectar declaraciones de variables locales dentro de funciones o bloques ---
    if (f_ast_is_var_decl_start(parser) || (parser->current_token.type == FOX_TOKEN_IDENTIFIER && f_ast_peek_is_identifier(parser))) {
        return f_ast_parse_var_declaration(parser, NULL, false);
    }

    // Redirigir sentencias conocidas a sus respectivas funciones
    switch (parser->current_token.type) {
        case FOX_TOKEN_KW_IF:
        case FOX_TOKEN_KW_ELSEIF:
            return f_ast_parse_if_statement(parser, NULL, false);
        case FOX_TOKEN_KW_WHILE:
            return f_ast_parse_while_statement(parser, NULL, false);
        case FOX_TOKEN_KW_FOR:
            return f_ast_parse_for_statement(parser, NULL, false);
        case FOX_TOKEN_KW_FOREACH:
            return f_ast_parse_foreach_statement(parser, NULL, false);
        case FOX_TOKEN_KW_SWITCH:
            return f_ast_parse_switch_statement(parser, NULL, false);
        case FOX_TOKEN_KW_CASE:
        case FOX_TOKEN_KW_DEFAULT:
            return f_ast_parse_case_statement(parser, NULL, false);
        case FOX_TOKEN_KW_RETURN:
            return f_ast_parse_return_statement(parser, NULL, false);
        case FOX_TOKEN_KW_BREAK:
            return f_ast_parse_break_statement(parser, NULL, false);
        case FOX_TOKEN_KW_CONTINUE:
            return f_ast_parse_continue_statement(parser, NULL, false);
        case FOX_TOKEN_KW_GOTO:
            return f_ast_parse_goto_statement(parser, NULL, false);
        case FOX_TOKEN_KW_TRY:
            return f_ast_parse_try_statement(parser, NULL, false);
        default:
            break;
    }

    return f_ast_parse_expression_statement(parser);
}

static FoxyAstNode *f_ast_parse_include(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->current_token.pos;

    // Consumir la palabra clave 'include'
    f_ast_consume(parser, FOX_TOKEN_KW_INCLUDE, "[DEBUG: f_ast_parse_include] Expected string literal or module path after 'include'.");

    FoxyToken path_token = parser->current_token;
    f_ast_advance(parser); // Avanzar pasando la ruta

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_INCLUDE, pos);
    node->as.include_stmt.path = path_token;
    return node;
}

static bool f_ast_peek_is_identifier(FoxyAstParser *parser) {
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
    
    FoxySourcePos start_pos = parser->current_token.pos;
    // =========================================================================
    // DEPURADOR PERSONALIZADO EN LÍNEA (VAR DECLARATION)
    // =========================================================================
    // printf("{\n"
    //        "\t[DEBUG VAR_DECL] line [%u]\n"
    //        "\t\tcurr_token: %s<%d>\n"
    //        "}\n",
    //        parser->current_token.pos.line,
    //        f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type
    // );
    
    bool is_global = false;
    bool is_static = false;
    bool is_const  = false;
    bool is_array  = false;
    FoxyAstNode *array_size = NULL;

    while (f_ast_var_is_calificator(parser->current_token.type)) {
        if (parser->current_token.type == FOX_TOKEN_KW_GLOBAL) is_global = true;
        if (parser->current_token.type == FOX_TOKEN_KW_STATIC) is_static = true;
        if (parser->current_token.type == FOX_TOKEN_KW_CONST)  is_const  = true;
        f_ast_advance(parser);
    }

    FoxyTokenType type_token = FOX_TOKEN_EOF;
    bool is_hybrid = true;

    if (f_ast_is_type_specifier(parser->current_token.type)) {
        type_token = parser->current_token.type;
        is_hybrid = false;
        f_ast_advance(parser);
    } else if (parser->current_token.type == FOX_TOKEN_IDENTIFIER && f_ast_peek_is_identifier(parser)) {
        type_token = parser->current_token.type; // Mantiene o registra el tipo personalizado
        is_hybrid = false;
        
        /* Registrar el identificador como tipo de dato conocido en el lexer */
        f_lexer_register_type(&parser->lexer, parser->current_token.start, parser->current_token.length);
        
        f_ast_advance(parser); // Consume el identificador del tipo (ej: Node)
    }

    if (parser->current_token.type != FOX_TOKEN_IDENTIFIER) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_var_declaration] Expected variable name.");
        return NULL;
    }

    FoxyToken name_token = parser->current_token;
    f_ast_advance(parser);

    if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
        is_array = true;
        f_ast_advance(parser);
        if (parser->current_token.type != FOX_TOKEN_RBRACKET) {
            array_size = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
        }
        if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_var_declaration] Expected ']' after array dimension.");
            if (array_size) f_ast_free_node(array_size);
            return NULL;
        }
    }

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
    node->as.var_decl.array_size = array_size;
    node->as.var_decl.is_array = is_array;
    node->as.var_decl.is_global = is_global;
    node->as.var_decl.is_static = is_static;
    node->as.var_decl.is_const = is_const;
    node->as.var_decl.is_hybrid = is_hybrid;

    return node;
}

static FoxyAstNode *f_ast_parse_declaration(FoxyAstParser *parser) {
    if (parser->current_token.type == FOX_TOKEN_EOF) {
        return NULL;
    }

    if (parser->current_token.type == FOX_TOKEN_KW_INCLUDE) {
        return f_ast_parse_include(parser, NULL, false);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_USE) {
        f_ast_advance(parser);
        return f_ast_parse_use_statement(parser, NULL, false);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_EXPORT) {
        f_ast_advance(parser);
        return f_ast_parse_declaration(parser);
    }

    if (f_ast_is_declarative(parser->current_token.type) || 
        parser->current_token.type == FOX_TOKEN_KW_OVERRULE) {
        f_ast_advance(parser);
        return f_ast_parse_declaration(parser);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_FUNCTION) {
        return f_ast_parse_function_declaration(parser, NULL, false);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_CLASS) {
        FoxyLexer lexer_copy = parser->lexer;
        FoxyToken next1 = f_lexer_next_token(&lexer_copy);
        bool is_class_def = (next1.type == FOX_TOKEN_LBRACE || next1.type == FOX_TOKEN_KW_FROM);
        if (!is_class_def && next1.type == FOX_TOKEN_IDENTIFIER) {
            FoxyToken next2 = f_lexer_next_token(&lexer_copy);
            is_class_def = (next2.type == FOX_TOKEN_LBRACE || next2.type == FOX_TOKEN_KW_FROM);
        }
        if (is_class_def) {
            // Registrar preventivamente el nombre de la clase si existe un identificador despues de 'class'
            FoxyLexer lookahead = parser->lexer;
            f_lexer_next_token(&lookahead); // avanza 'class'
            if (lookahead.cursor && lookahead.token_start) {
                // O de forma más limpia usando el estado del parser tras verificar el nombre:
            }
            // Alternativa directa inspeccionando el siguiente token del parser:
            // (Nota: puedes registrar el token si viene un identificador antes de '{' o 'from')
            return f_ast_parse_class_declaration(parser, NULL, false);
        }
    }

    if (parser->current_token.type == FOX_TOKEN_KW_STRUCT) {
        FoxyLexer lexer_copy = parser->lexer;
        FoxyToken next1 = f_lexer_next_token(&lexer_copy);
        bool is_struct_def = (next1.type == FOX_TOKEN_LBRACE);
        if (!is_struct_def && next1.type == FOX_TOKEN_IDENTIFIER) {
            // Si el siguiente es un identificador, es el nombre del struct (ej: struct Node)
            f_lexer_register_type(&parser->lexer, next1.start, next1.length);
            FoxyToken next2 = f_lexer_next_token(&lexer_copy);
            is_struct_def = (next2.type == FOX_TOKEN_LBRACE);
        }
        if (is_struct_def) {
            return f_ast_parse_struct_declaration(parser, NULL, false);
        }
    }

    if (parser->current_token.type == FOX_TOKEN_KW_ENUM) {
        FoxyLexer lexer_copy = parser->lexer;
        FoxyToken next1 = f_lexer_next_token(&lexer_copy);
        bool is_enum_def = (next1.type == FOX_TOKEN_LBRACE);
        if (!is_enum_def && next1.type == FOX_TOKEN_IDENTIFIER) {
            f_lexer_register_type(&parser->lexer, next1.start, next1.length);
            FoxyToken next2 = f_lexer_next_token(&lexer_copy);
            is_enum_def = (next2.type == FOX_TOKEN_LBRACE);
        }
        if (is_enum_def) {
            return f_ast_parse_enum_declaration(parser, NULL, false);
        }
    }

    // Si comienza con un calificador, tipo primitivo o tipo personalizado seguido de identificador
    if (f_ast_is_var_decl_start(parser)) {
        return f_ast_parse_var_declaration(parser, NULL, false);
    }

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
        fprintf(stderr, "[Foxy Parser Error] Line %u: [DEBUG: f_ast_parse_assign] Invalid assignment target.\n", parser->previous_token.pos.line);
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
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_call] Expected ')' after function arguments.");
        f_ast_free_node(node);
        return NULL;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_dict(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    FoxyAstNode *dict_node = f_ast_create_node(FOXY_AST_EXPR_DICT_LITERAL, pos);
    f_ast_node_list_init(&dict_node->as.dict_literal.entries);

    while (parser->current_token.type != FOX_TOKEN_RBRACE && parser->current_token.type != FOX_TOKEN_EOF) {
        FoxyToken key_token = {0};

        if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
            f_ast_advance(parser); // Consumir '['
            
            // Guardar el token de la clave numérica o identificador dentro de los corchetes
            if (parser->current_token.type == FOX_TOKEN_INT_LITERAL || 
                parser->current_token.type == FOX_TOKEN_STRING_LITERAL || 
                parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                key_token = parser->current_token;
            }

            FoxyAstNode *key_expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
            if (key_expr) f_ast_free_node(key_expr); // Liberar expresión temporal si se usó solo para valor
            
            if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                f_ast_advance(parser); // Consumir ']'
            } else {
                f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_dict] Expected ']' after dictionary key.");
                f_ast_free_node(dict_node);
                return NULL;
            }
        } else {
            key_token = parser->current_token;
            f_ast_advance(parser);
        }

        if (parser->current_token.type == FOX_TOKEN_ASSIGN) {
            f_ast_advance(parser); // Consumir '='
        } else {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_dict] Expected '=' after dictionary key.");
            f_ast_free_node(dict_node);
            return NULL;
        }

        FoxyAstNode *val = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
        FoxyAstNode *entry = f_ast_create_node(FOXY_AST_EXPR_DICT_ENTRY, key_token.pos);
        entry->as.dict_entry.key = key_token;
        entry->as.dict_entry.value = val;
        f_ast_node_list_append(&dict_node->as.dict_literal.entries, entry);

        if (parser->current_token.type == FOX_TOKEN_COMMA) {
            f_ast_advance(parser);
        } else {
            break;
        }
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_dict] Expected '}' after dictionary entries.");
    }

    return dict_node;
}

static FoxyAstNode *f_ast_parse_array(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_ARRAY_LITERAL, pos);
    f_ast_node_list_init(&node->as.array_literal.elements);

    if (parser->current_token.type != FOX_TOKEN_RBRACE) {
        do {
            if (parser->current_token.type == FOX_TOKEN_RBRACE || parser->current_token.type == FOX_TOKEN_EOF) break;
            
            FoxyAstNode *elem = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
            if (elem) {
                f_ast_node_list_append(&node->as.array_literal.elements, elem);
            }
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true) && parser->current_token.type != FOX_TOKEN_RBRACE);
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser); // Consumir '}'
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_array] Expected '}' after array literal elements.");
    }

    return node;
}

static FoxyAstNode *f_ast_parse_dict_or_array(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        return f_ast_parse_array(parser, left, can_assign);
    }

    FoxyLexer lookahead_lexer = parser->lexer;
    FoxyToken first_tok = parser->current_token;
    bool is_dict = false;

    // Soportar tanto [key] = val como key = val
    if (first_tok.type == FOX_TOKEN_LBRACKET) {
        int bracket_depth = 1;
        FoxyToken t = f_lexer_next_token(&lookahead_lexer);
        while (bracket_depth > 0 && t.type != FOX_TOKEN_EOF) {
            if (t.type == FOX_TOKEN_LBRACKET) bracket_depth++;
            else if (t.type == FOX_TOKEN_RBRACKET) bracket_depth--;
            if (bracket_depth == 0) break;
            t = f_lexer_next_token(&lookahead_lexer);
        }
        FoxyToken after_bracket = f_lexer_next_token(&lookahead_lexer);
        if (after_bracket.type == FOX_TOKEN_ASSIGN) {
            is_dict = true;
        }
    } else {
        FoxyToken next_token = f_lexer_next_token(&lookahead_lexer);
        if (next_token.type == FOX_TOKEN_ASSIGN) {
            is_dict = true;
        }
    }

    if (is_dict) {
        return f_ast_parse_dict(parser, left, can_assign);
    }

    return f_ast_parse_array(parser, left, can_assign);
}

static FoxyAstNode *f_ast_parse_index(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;
    FoxyAstNode *index_expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_index] Expected ']' after array index expression.");
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
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_ternary] Expected ':' in ternary operator.");
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
    if (parser->current_token.type == FOX_TOKEN_EOF || parser->previous_token.type == FOX_TOKEN_EOF) {
        return NULL;
    }
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
                f_ast_advance(parser);
            }
        }
    }

    if (parser->current_token.type == FOX_TOKEN_RBRACE) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_block_statement] Expected '}' after block.");
    }

    return node;
}

static FoxyAstNode *f_ast_parse_if_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_IF || parser->current_token.type == FOX_TOKEN_KW_ELSEIF) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;
    
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
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_if_statement] Expected ')' after condition.");
            f_ast_free_node(condition);
            return NULL;
        }
    }

    FoxyAstNode *then_branch = f_ast_parse_statement(parser);
    FoxyAstNode *else_branch = NULL;

    if (parser->current_token.type == FOX_TOKEN_KW_ELSEIF) {
        else_branch = f_ast_parse_statement(parser);
    } else if (parser->current_token.type == FOX_TOKEN_KW_ELSE) {
        f_ast_advance(parser);
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


    if (parser->current_token.type == FOX_TOKEN_KW_RETURN) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;
    FoxyAstNode *value = NULL;

    if (parser->current_token.type != FOX_TOKEN_SEMICOLON) {
        value = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

        while (parser->current_token.type == FOX_TOKEN_COMMA) {
            FoxyToken comma_op = parser->current_token;
            f_ast_advance(parser);
            FoxyAstNode *next_expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
            if (value && next_expr) {
                value = f_ast_create_binary_node(comma_op, value, next_expr);
            }
        }
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
    FoxySourcePos pos = parser->previous_token.pos;

    // =========================================================================
    // DEPURADOR PERSONALIZADO EN LÍNEA (FUNCTION DECLARATION)
    // =========================================================================
    // printf("{\n"
    //        "\t[DEBUG FUNC_DECL] line [%u]\n"
    //        "\t\tprev_token: %s<%d> | curr_token: %s<%d>\n"
    //        "}\n",
    //        parser->current_token.pos.line,
    //        f_lexer_token_type_to_string(parser->previous_token.type), parser->previous_token.type,
    //        f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type
    // );

    // Consumir 'function' si venimos desde el token previo o actual de Pratt
    if (parser->previous_token.type == FOX_TOKEN_KW_FUNCTION) {
        // Ya fue avanzado por el dispatcher de Pratt
    } else if (parser->current_token.type == FOX_TOKEN_KW_FUNCTION) {
        f_ast_advance(parser);
    }

    bool is_anonymous = false;
    FoxyToken name = {0};

    // Si el siguiente token es '(', es una función anónima estilo Lua: function(...)
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        is_anonymous = true;
    } else {
        if (!f_ast_is_valid_func_name(parser)) {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_function_declaration] Expected function name or '(' for anonymous function.");
            return NULL;
        }
        name = parser->current_token;
        f_ast_advance(parser);
    }

    if (parser->current_token.type != FOX_TOKEN_LPAREN) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_function_declaration] Expected '(' after function name or signature.");
        f_ast_synchronize(parser);
        return NULL;
    }
    f_ast_advance(parser);

    FoxyAstNodeList params;
    f_ast_node_list_init(&params);

    if (parser->current_token.type != FOX_TOKEN_RPAREN) {
        do {
            if (f_ast_is_type_specifier(parser->current_token.type) || (parser->current_token.type == FOX_TOKEN_IDENTIFIER && f_ast_peek_is_identifier(parser))) {
                f_ast_advance(parser);

                if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
                    f_ast_advance(parser);
                    if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                        f_ast_advance(parser);
                    }
                }
            }

            if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                FoxyAstNode *param = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->current_token.pos);
                param->as.identifier.name = parser->current_token;
                f_ast_node_list_append(&params, param);
                f_ast_advance(parser);

                if (parser->current_token.type == FOX_TOKEN_DOTDOTDOT) {
                    f_ast_advance(parser);
                }
            } else if (parser->current_token.type == FOX_TOKEN_DOTDOTDOT) {
                f_ast_advance(parser);
                if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                    FoxyAstNode *param = f_ast_create_node(FOXY_AST_EXPR_IDENTIFIER, parser->current_token.pos);
                    param->as.identifier.name = parser->current_token;
                    f_ast_node_list_append(&params, param);
                    f_ast_advance(parser);
                }
            } else {
                f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_function_declaration] Expected parameter name.");
                f_ast_free_node_list(&params);
                f_ast_synchronize(parser);
                return NULL;
            }
        } while (parser->current_token.type == FOX_TOKEN_COMMA && (f_ast_advance(parser), true));
    }

    if (parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_function_declaration] Expected ')' after parameter list.");
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
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_function_declaration] Expected '{' or ';' after function signature.");
        f_ast_free_node_list(&params);
        f_ast_synchronize(parser);
        return NULL;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_FUNC_DECL, pos);
    node->as.func_decl.name = name;
    node->as.func_decl.params = params;
    node->as.func_decl.body = body;
    
    (void)is_anonymous; // Evita advertencias si aún no agregas el bit/campo al struct

    return node;
}

static FoxyAstNode *f_ast_parse_while_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_WHILE) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;

    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    FoxyAstNode *condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);

    if (has_paren) {
        f_ast_consume(parser, FOX_TOKEN_RPAREN, "[DEBUG: f_ast_parse_while_statement] Expected ')' after while condition.");
    }

    FoxyAstNode *body = f_ast_parse_statement(parser);
    
    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_WHILE, pos);
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

static FoxyAstNode *f_ast_parse_for_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_FOR) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;

    bool has_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_paren = true;
        f_ast_advance(parser);
    }

    FoxyAstNode *init = NULL;
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    } else if (f_ast_is_var_decl_start(parser) || (parser->current_token.type == FOX_TOKEN_IDENTIFIER && f_ast_peek_is_identifier(parser))) {
        init = f_ast_parse_var_declaration(parser, NULL, false);
    } else {
        init = f_ast_parse_expression_statement(parser);
    }

    FoxyAstNode *condition = NULL;
    if (parser->current_token.type != FOX_TOKEN_SEMICOLON) {
        condition = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    FoxyAstNode *increment = NULL;
    if (parser->current_token.type != FOX_TOKEN_RPAREN && parser->current_token.type != FOX_TOKEN_LBRACE) {
        increment = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    if (has_paren) {
        if (parser->current_token.type == FOX_TOKEN_RPAREN) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_for_statement] Expected ')' after for clauses.");
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

    if (parser->current_token.type == FOX_TOKEN_KW_FOREACH) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;

    bool has_outer_paren = false;
    if (parser->current_token.type == FOX_TOKEN_LPAREN) {
        has_outer_paren = true;
        f_ast_advance(parser);
    }

    FoxyToken iterator_var = {0};

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

    if (has_outer_paren && parser->current_token.type == FOX_TOKEN_RPAREN) {
        f_ast_advance(parser);
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


    if (parser->current_token.type == FOX_TOKEN_KW_SWITCH) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;

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
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_switch_statement] Expected ')' after switch condition.");
            f_ast_free_node(condition);
            return NULL;
        }
    }

    if (parser->current_token.type != FOX_TOKEN_LBRACE) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_switch_statement] Expected '{' before switch body.");
        f_ast_free_node(condition);
        return NULL;
    }
    f_ast_advance(parser);

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
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_switch_statement] Expected '}' after switch body.");
        f_ast_free_node(node);
        return NULL;
    }

    return node;
}

static FoxyAstNode *f_ast_parse_case_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    FoxySourcePos pos = parser->previous_token.pos;
    bool is_default = (parser->previous_token.type == FOX_TOKEN_KW_DEFAULT);

    FoxyAstNode *expr = NULL;
    if (!is_default) {
        expr = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
        if (!expr) return NULL;
    }

    if (parser->current_token.type == FOX_TOKEN_COLON) {
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_case_statement] Expected ':' after case/default expression.");
        if (expr) f_ast_free_node(expr);
        return NULL;
    }

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_CASE, pos);
    node->as.case_stmt.expr = expr;
    f_ast_node_list_init(&node->as.case_stmt.stmts);

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


    if (parser->current_token.type == FOX_TOKEN_KW_BREAK) {
        f_ast_advance(parser);
    }

    FoxySourcePos pos = parser->previous_token.pos;

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    return f_ast_create_node(FOXY_AST_STMT_BREAK, pos);
}

static FoxyAstNode *f_ast_parse_continue_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_CONTINUE) {
        f_ast_advance(parser);
    }

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

    if (parser->current_token.type == FOX_TOKEN_KW_GOTO) {
        f_ast_advance(parser);
    }

    if (parser->current_token.type != FOX_TOKEN_IDENTIFIER && 
        parser->current_token.type != FOX_TOKEN_LABEL) {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_goto_statement] Expected label identifier after 'goto'.");
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

    FoxySourcePos pos = parser->previous_token.pos;

    if (parser->current_token.type == FOX_TOKEN_KW_TRY) {
        f_ast_advance(parser);
    }

    FoxyAstNode *try_block = f_ast_parse_statement(parser);

    FoxyAstNode *try_node = f_ast_create_node(FOXY_AST_STMT_TRY, pos);
    try_node->as.try_stmt.try_block = try_block;
    try_node->as.try_stmt.finally_block = NULL;
    f_ast_node_list_init(&try_node->as.try_stmt.catch_blocks);

    while (parser->current_token.type == FOX_TOKEN_KW_CATCH || parser->current_token.type == FOX_TOKEN_KW_EXCEPT) {
        FoxySourcePos catch_pos = parser->current_token.pos;
        f_ast_advance(parser);

        FoxyToken var_name = (FoxyToken){0};
        if (parser->current_token.type == FOX_TOKEN_LPAREN) {
            f_ast_advance(parser);
            
            if (f_ast_is_type_specifier(parser->current_token.type)) {
                f_ast_advance(parser);
            }

            if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
                var_name = parser->current_token;
                f_ast_advance(parser);
            }

            if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
                f_ast_advance(parser);
                if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
                    f_ast_advance(parser);
                }
            }

            if (parser->current_token.type == FOX_TOKEN_RPAREN) {
                f_ast_advance(parser);
            } else {
                f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_try_statement] Expected ')' after catch variable.");
                f_ast_advance(parser);
            }
        }

        FoxyAstNode *catch_body = f_ast_parse_statement(parser);

        FoxyAstNode *catch_node = f_ast_create_node(FOXY_AST_STMT_CATCH, catch_pos);
        catch_node->as.catch_stmt.var_name = var_name;
        catch_node->as.catch_stmt.body = catch_body;

        f_ast_node_list_append(&try_node->as.try_stmt.catch_blocks, catch_node);
    }

    if (parser->current_token.type == FOX_TOKEN_KW_FINAL) {
        f_ast_advance(parser);
        try_node->as.try_stmt.finally_block = f_ast_parse_statement(parser);
    }

    return try_node;
}

static FoxyAstNode *f_ast_parse_unpack(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
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

static FoxyAstNode *f_ast_parse_export_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    if (parser->current_token.type == FOX_TOKEN_KW_EXPORT) {
        f_ast_advance(parser);
    }
    return f_ast_parse_declaration(parser);
}

static FoxyAstNode *f_ast_parse_use_statement(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;

    while (parser->current_token.type != FOX_TOKEN_SEMICOLON && 
           parser->current_token.type != FOX_TOKEN_EOF && 
           parser->current_token.pos.line == parser->previous_token.pos.line) {
        f_ast_advance(parser);
    }
    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }
    return NULL;
}

static FoxyAstNode *f_ast_parse_enum_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    if (parser->current_token.type == FOX_TOKEN_KW_ENUM) {
        f_ast_advance(parser);
    }

    /* Registrar opcionalmente el enum si tiene nombre explícito */
    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        f_lexer_register_type(&parser->lexer, parser->current_token.start, parser->current_token.length);
        f_ast_advance(parser);
    }

    FoxyAstNode *block_node = f_ast_create_node(FOXY_AST_STMT_BLOCK, pos);
    f_ast_node_list_init(&block_node->as.block_stmt.statements);

    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);

        int current_val = 0;
        while (parser->current_token.type != FOX_TOKEN_RBRACE && 
               parser->current_token.type != FOX_TOKEN_EOF) {

            if (parser->current_token.type != FOX_TOKEN_IDENTIFIER) {
                f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_enum_declaration] Expected enum item identifier.");
                f_ast_synchronize(parser);
                break;
            }

            FoxyToken item_name = parser->current_token;
            FoxySourcePos item_pos = item_name.pos;
            f_ast_advance(parser);

            FoxyAstNode *initializer = NULL;

            if (parser->current_token.type == FOX_TOKEN_ASSIGN) {
                f_ast_advance(parser);
                initializer = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
                if (initializer && initializer->kind == FOXY_AST_EXPR_LITERAL &&
                    initializer->as.literal.value.type == FOXY_AST_VAL_INT) {
                    current_val = initializer->as.literal.value.as.f_int + 1;
                } else {
                    current_val++;
                }
            } else {
                FoxyAstNode *lit = f_ast_create_node(FOXY_AST_EXPR_LITERAL, item_pos);
                lit->as.literal.value = f_ast_new_int(current_val++);
                initializer = lit;
            }

            FoxyAstNode *item_node = f_ast_create_node(FOXY_AST_STMT_VAR_DECL, item_pos);
            item_node->as.var_decl.name = item_name;
            item_node->as.var_decl.type_token = FOX_TOKEN_KW_INT;
            item_node->as.var_decl.initializer = initializer;
            item_node->as.var_decl.is_const = 1;
            item_node->as.var_decl.is_static = 1;

            f_ast_node_list_append(&block_node->as.block_stmt.statements, item_node);

            if (parser->current_token.type == FOX_TOKEN_COMMA) {
                f_ast_advance(parser);
            } else if (parser->current_token.type != FOX_TOKEN_RBRACE) {
                f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_enum_declaration] Expected ',' or '}' in enum declaration.");
                f_ast_synchronize(parser);
                break;
            }
        }

        if (parser->current_token.type == FOX_TOKEN_RBRACE) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_enum_declaration] Expected '}' after enum body.");
        }
    }

    if (parser->current_token.type == FOX_TOKEN_SEMICOLON) {
        f_ast_advance(parser);
    }

    return block_node;
}

static FoxyAstNode *f_ast_parse_class_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_CLASS) {
        f_ast_advance(parser);
    }

    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        f_lexer_register_type(&parser->lexer, parser->current_token.start, parser->current_token.length);
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_class_declaration] Expected class name.");
        return NULL;
    }

    if (parser->current_token.type == FOX_TOKEN_KW_FROM) {
        f_ast_advance(parser);
        if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
            f_ast_advance(parser);
        } else {
            f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_class_declaration] Expected parent class name after 'from'.");
            return NULL;
        }
    }

    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, false);
    }

    f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_class_declaration] Expected '{' before class body.");
    return NULL;
}

static FoxyAstNode *f_ast_parse_struct_declaration(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;


    if (parser->current_token.type == FOX_TOKEN_KW_STRUCT) {
        f_ast_advance(parser);
    }

    if (parser->current_token.type == FOX_TOKEN_IDENTIFIER) {
        /* Registrar automáticamente el identificador como tipo de dato conocido */
        f_lexer_register_type(&parser->lexer, parser->current_token.start, parser->current_token.length);
        f_ast_advance(parser);
    } else {
        f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_struct_declaration] Expected struct name.");
        return NULL;
    }

    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        return f_ast_parse_block_statement(parser, NULL, false);
    }

    f_ast_error_at_current(parser, "[DEBUG: f_ast_parse_struct_declaration] Expected '{' before struct body.");
    return NULL;
}

static FoxyAstNode *f_ast_parse_lambda(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)can_assign;
    FoxySourcePos pos = parser->previous_token.pos;

    // =========================================================================
    // DEPURADOR PERSONALIZADO EN LÍNEA (LAMBDA)
    // =========================================================================
    // printf("{\n"
    //        "\t[DEBUG LAMBDA] line [%u]\n"
    //        "\t\tprev_token: %s<%d> | curr_token: %s<%d>\n"
    //        "\t\thas_left_node: %s\n"
    //        "}\n",
    //        parser->current_token.pos.line,
    //        f_lexer_token_type_to_string(parser->previous_token.type), parser->previous_token.type,
    //        f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type,
    //        left != NULL ? "true" : "false"
    // );

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_STMT_LAMBDA, pos);
    f_ast_node_list_init(&node->as.lambda_stmt.params);

    // Extraer parámetros del nodo izquierdo generado en el grupo (ej: (int x, int y))
    if (left != NULL) {
        if (left->kind == FOXY_AST_STMT_BLOCK) {
            for (size_t i = 0; i < left->as.block_stmt.statements.count; ++i) {
                FoxyAstNode *param = left->as.block_stmt.statements.nodes[i];
                f_ast_node_list_append(&node->as.lambda_stmt.params, param);
            }
            free(left->as.block_stmt.statements.nodes);
            free(left);
        } else {
            f_ast_node_list_append(&node->as.lambda_stmt.params, left);
        }
    }

    FoxyAstNode *body = NULL;
    if (parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser);
        body = f_ast_parse_block_statement(parser, NULL, false);
    } else {
        body = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
    }

    node->as.lambda_stmt.body = body;

    return node;
}

static FoxyAstNode *f_ast_parse_type_prefix(FoxyAstParser *parser, FoxyAstNode *left, bool can_assign) {
    (void)left;
    (void)can_assign;
    FoxyToken type_token = parser->previous_token;
    FoxySourcePos pos = type_token.pos;

    // =========================================================================
    // DEPURADOR PERSONALIZADO EN LÍNEA (TYPE PREFIX / CAST)
    // =========================================================================
    // printf("{\n"
    //        "\t[DEBUG TYPE_PREFIX] line [%u]\n"
    //        "\t\ttype_token: %s<%d> | curr_token: %s<%d>\n"
    //        "}\n",
    //        parser->current_token.pos.line,
    //        f_lexer_token_type_to_string(type_token.type), type_token.type,
    //        f_lexer_token_type_to_string(parser->current_token.type), parser->current_token.type
    // );

    bool is_array_cast = false;
    FoxyAstNode *array_size = NULL;

    // Verificar si viene seguido de corchetes, ej: int[] o int[10]
    if (parser->current_token.type == FOX_TOKEN_LBRACKET) {
        f_ast_advance(parser); // Consumir '['
        is_array_cast = true;
        
        if (parser->current_token.type != FOX_TOKEN_RBRACKET) {
            array_size = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
        }
        
        if (parser->current_token.type == FOX_TOKEN_RBRACKET) {
            f_ast_advance(parser); // Consumir ']'
        } else {
            f_ast_error_at_current(parser, "Expected ']' after array dimension in type.");
            if (array_size) f_ast_free_node(array_size);
            return NULL;
        }
    }

    // Si va seguido de llaves { ... }, es un arreglo literal casteado (ej: int[]{1, 2, 3})
    if (is_array_cast && parser->current_token.type == FOX_TOKEN_LBRACE) {
        f_ast_advance(parser); // Consumir '{'

        FoxyAstNode *array_node = f_ast_create_node(FOXY_AST_EXPR_ARRAY_LITERAL, pos);
        array_node->as.array_literal.token_type = type_token.type;
        f_ast_node_list_init(&array_node->as.array_literal.elements);

        if (parser->current_token.type != FOX_TOKEN_RBRACE) {
            do {
                if (parser->current_token.type == FOX_TOKEN_RBRACE || parser->current_token.type == FOX_TOKEN_EOF) break;
                
                FoxyAstNode *elem = f_ast_parse_precedence(parser, PREC_ASSIGNMENT);
                if (elem) {
                    f_ast_node_list_append(&array_node->as.array_literal.elements, elem);
                }
                
                if (parser->current_token.type == FOX_TOKEN_COMMA) {
                    f_ast_advance(parser);
                } else {
                    break;
                }
            } while (parser->current_token.type != FOX_TOKEN_RBRACE && parser->current_token.type != FOX_TOKEN_EOF);
        }

        if (parser->current_token.type == FOX_TOKEN_RBRACE) {
            f_ast_advance(parser); // Consumir '}'
        } else {
            f_ast_error_at_current(parser, "Expected '}' after array literal elements.");
        }

        return array_node;
    }

    // De lo contrario, es un casteo simple de un valor (ej: int 97, o manejado tras un paréntesis)
    FoxyAstNode *operand = f_ast_parse_precedence(parser, PREC_UNARY);
    if (!operand) return NULL;

    FoxyAstNode *node = f_ast_create_node(FOXY_AST_EXPR_UNARY, pos);
    node->as.unary.op = type_token;
    node->as.unary.operand = operand;
    node->as.unary.is_postfix = 0;
    return node;
}