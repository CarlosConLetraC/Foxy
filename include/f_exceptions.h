#pragma once

#include "f_settings.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ---------------- 1. Declaración de Enums ---------------- */
#if FOXY_COMPILER_SUPPORTS_XMACROS

#define FOXY_EXCEPTION_LIST(F) \
    /* --- Errores Léxicos / Lexer --- */ \
    F(FOX_EXCEPTION_UNTERMINATED_STRING = 1) \
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

typedef enum FOXY_PACKED {
    FOX_EXCEPTION_UNTERMINATED_STRING = 1,
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

/* ---------------- 2. Controlador de Excepciones Unificado ---------------- */

struct FoxyExceptionController {
    uint32_t error_count;
    uint32_t warning_count;
    bool panic_mode;
    bool had_error;
};

typedef struct FoxyExceptionController FoxyExceptionController;

/* ---------------- 3. Tabla LUT en Crudo ---------------- */
extern const char *const FOXY_EXCEPTION_MESSAGES[FOX_EXCEPTION_COUNT];

/* ---------------- 4. API del Controlador ---------------- */
FOXY_EXPORT void f_exception_controller_init(FoxyExceptionController *controller);
FOXY_EXPORT int f_exception_raise(FoxyExceptionController *controller, FoxyExceptionCode code, const char *filename, uint32_t line, uint32_t column, const char *format, ...);
FOXY_EXPORT void f_exception_clear(FoxyExceptionController *controller);