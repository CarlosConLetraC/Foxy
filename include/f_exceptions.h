#pragma once

#include "f_settings.h"

/* ---------------- 1. Declaración de Enums ---------------- */
#if FOXY_COMPILER_SUPPORTS_XMACROS

/* La X-Macro solo proyecta la lista de identificadores */
#define FOXY_EXCEPTION_LIST(F) \
    /* --- Errores Léxicos / Lexer --- */ \
    F(FOX_EXCEPTION_UNTERMINATED_STRING) \
    F(FOX_EXCEPTION_UNTERMINATED_BLOCK_COMMENT) \
    F(FOX_EXCEPTION_MALFORMED_NUMBER) \
    F(FOX_EXCEPTION_INVALID_CHAR) \
    \
    /* --- Errores Sintácticos / Parser --- */ \
    F(FOX_EXCEPTION_SYNTAX_ERROR) \
    F(FOX_EXCEPTION_EXPECTED_TOKEN) \
    F(FOX_EXCEPTION_UNBALANCED_PAREN) \
    F(FOX_EXCEPTION_INVALID_ASSIGNMENT_TARGET) \
    \
    /* --- Errores Semánticos / Comprobación de Tipos --- */ \
    F(FOX_EXCEPTION_TYPE_MISMATCH) \
    F(FOX_EXCEPTION_UNDECLARED_IDENTIFIER) \
    F(FOX_EXCEPTION_REDECLARATION_ERROR) \
    F(FOX_EXCEPTION_IMMUTABLE_ASSIGNMENT) \
    F(FOX_EXCEPTION_ARITY_MISMATCH) \
    \
    /* --- Errores de Tiempo de Ejecución / Runtime --- */ \
    F(FOX_EXCEPTION_ZERO_DIVISION) \
    F(FOX_EXCEPTION_INDEX_OUT_OF_BOUNDS) \
    F(FOX_EXCEPTION_NULL_POINTER_DEREF) \
    F(FOX_EXCEPTION_STACK_OVERFLOW) \
    F(FOX_EXCEPTION_OUT_OF_MEMORY) \
    F(FOX_EXCEPTION_BAD_CAST) \
    F(FOX_EXCEPTION_KEY_NOT_FOUND) \
    F(FOX_EXCEPTION_OVERFLOW) \
    F(FOX_EXCEPTION_UNDERFLOW) \
    \
    /* --- Entrada/Salida y Sistema (I/O & OS) --- */ \
    F(FOX_EXCEPTION_FILE_NOT_FOUND) \
    F(FOX_EXCEPTION_IO_ERROR) \
    F(FOX_EXCEPTION_PERMISSION_DENIED) \
    \
    /* --- Módulos / Carga Dinámica --- */ \
    F(FOX_EXCEPTION_MODULE_NOT_FOUND) \
    F(FOX_EXCEPTION_SYMBOL_NOT_FOUND)

typedef enum FOXY_PACKED {
#define DEFINE_ENUM(code) code,
    FOXY_EXCEPTION_LIST(DEFINE_ENUM)
#undef DEFINE_ENUM
    FOX_EXCEPTION_COUNT
} FoxyExceptionCode;

#else

/* Fallback manual indexado idénticamente */
typedef enum FOXY_PACKED {
    FOX_EXCEPTION_UNTERMINATED_STRING,
    FOX_EXCEPTION_UNTERMINATED_BLOCK_COMMENT,
    FOX_EXCEPTION_MALFORMED_NUMBER,
    FOX_EXCEPTION_INVALID_CHAR,
    FOX_EXCEPTION_SYNTAX_ERROR,
    FOX_EXCEPTION_EXPECTED_TOKEN,
    FOX_EXCEPTION_UNBALANCED_PAREN,
    FOX_EXCEPTION_INVALID_ASSIGNMENT_TARGET,
    FOX_EXCEPTION_TYPE_MISMATCH,
    FOX_EXCEPTION_UNDECLARED_IDENTIFIER,
    FOX_EXCEPTION_REDECLARATION_ERROR,
    FOX_EXCEPTION_IMMUTABLE_ASSIGNMENT,
    FOX_EXCEPTION_ARITY_MISMATCH,
    FOX_EXCEPTION_ZERO_DIVISION,
    FOX_EXCEPTION_INDEX_OUT_OF_BOUNDS,
    FOX_EXCEPTION_NULL_POINTER_DEREF,
    FOX_EXCEPTION_STACK_OVERFLOW,
    FOX_EXCEPTION_OUT_OF_MEMORY,
    FOX_EXCEPTION_BAD_CAST,
    FOX_EXCEPTION_KEY_NOT_FOUND,
    FOX_EXCEPTION_OVERFLOW,
    FOX_EXCEPTION_UNDERFLOW,
    FOX_EXCEPTION_FILE_NOT_FOUND,
    FOX_EXCEPTION_IO_ERROR,
    FOX_EXCEPTION_PERMISSION_DENIED,
    FOX_EXCEPTION_MODULE_NOT_FOUND,
    FOX_EXCEPTION_SYMBOL_NOT_FOUND,
    FOX_EXCEPTION_COUNT
} FoxyExceptionCode;

#endif

/* ---------------- 2. Tabla LUT en Crudo (Única Fuente de Cadenas) ---------------- */
static const char *const FOXY_EXCEPTION_MESSAGES[FOX_EXCEPTION_COUNT] = {
    [FOX_EXCEPTION_UNTERMINATED_STRING]        = "unterminated string literal",
    [FOX_EXCEPTION_UNTERMINATED_BLOCK_COMMENT] = "unterminated block comment",
    [FOX_EXCEPTION_MALFORMED_NUMBER]           = "malformed numeric literal or invalid suffix",
    [FOX_EXCEPTION_INVALID_CHAR]               = "unexpected or invalid character",
    [FOX_EXCEPTION_SYNTAX_ERROR]               = "syntax error",
    [FOX_EXCEPTION_EXPECTED_TOKEN]             = "expected token was not found",
    [FOX_EXCEPTION_UNBALANCED_PAREN]           = "unbalanced parenthesis or delimiter",
    [FOX_EXCEPTION_INVALID_ASSIGNMENT_TARGET]  = "invalid lvalue in assignment",
    [FOX_EXCEPTION_TYPE_MISMATCH]              = "type mismatch in expression or assignment",
    [FOX_EXCEPTION_UNDECLARED_IDENTIFIER]      = "use of undeclared variable or identifier",
    [FOX_EXCEPTION_REDECLARATION_ERROR]        = "redeclaration of existing symbol",
    [FOX_EXCEPTION_IMMUTABLE_ASSIGNMENT]       = "cannot assign to immutable symbol or constant",
    [FOX_EXCEPTION_ARITY_MISMATCH]             = "incorrect number of arguments passed to function",
    [FOX_EXCEPTION_ZERO_DIVISION]              = "division by zero",
    [FOX_EXCEPTION_INDEX_OUT_OF_BOUNDS]        = "index out of bounds",
    [FOX_EXCEPTION_NULL_POINTER_DEREF]         = "dereference of null reference or pointer",
    [FOX_EXCEPTION_STACK_OVERFLOW]             = "call stack limit exceeded",
    [FOX_EXCEPTION_OUT_OF_MEMORY]              = "memory allocation failure",
    [FOX_EXCEPTION_BAD_CAST]                   = "invalid type cast or conversion",
    [FOX_EXCEPTION_KEY_NOT_FOUND]              = "key not found in map or dictionary",
    [FOX_EXCEPTION_OVERFLOW]                   = "numeric calculation overflow",
    [FOX_EXCEPTION_UNDERFLOW]                  = "numeric calculation underflow",
    [FOX_EXCEPTION_FILE_NOT_FOUND]             = "file not found or cannot be opened",
    [FOX_EXCEPTION_IO_ERROR]                   = "input/output stream failure",
    [FOX_EXCEPTION_PERMISSION_DENIED]          = "access or operation permission denied",
    [FOX_EXCEPTION_MODULE_NOT_FOUND]           = "module or package could not be resolved",
    [FOX_EXCEPTION_SYMBOL_NOT_FOUND]           = "exported symbol not found in module"
};

/* ---------------- 3. Lookup Unificado O(1) ---------------- */
static inline const char* f_exception_str(FoxyExceptionCode code) {
    if ((unsigned int)code < (unsigned int)FOX_EXCEPTION_COUNT) {
        const char *msg = FOXY_EXCEPTION_MESSAGES[code];
        return msg ? msg : "<unknown exception>";
    }
    return "<unknown exception>";
}