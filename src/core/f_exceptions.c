#include "f_exceptions.h"
#include <stdio.h>
#include <stdarg.h>

const char *const FOXY_EXCEPTION_MESSAGES[FOX_EXCEPTION_COUNT] = {
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

static inline const char* f_exception_str(FoxyExceptionCode code) {
    if ((unsigned int)code < (unsigned int)FOX_EXCEPTION_COUNT) {
        const char *msg = FOXY_EXCEPTION_MESSAGES[code];
        return msg ? msg : "<unknown exception>";
    }
    return "<unknown exception>";
}

FOXY_EXPORT void f_exception_controller_init(FoxyExceptionController *controller) {
    if (!controller) return;
    controller->error_count = 0;
    controller->warning_count = 0;
    controller->panic_mode = false;
    controller->had_error = false;
}

FOXY_EXPORT int f_exception_raise(FoxyExceptionController *controller, FoxyExceptionCode code, const char *filename, uint32_t line, uint32_t column, const char *format, ...) {
    if (controller) {
        controller->error_count++;
        controller->had_error = true;
    }

    fprintf(stderr, "[Foxy Error] %s:%u:%u: [%s] ", 
        filename ? filename : "<unknown>", 
        line, 
        column, 
        f_exception_str(code)
    );
    
    if (format) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
    
    fprintf(stderr, "\n");
    return (int)code;
}

FOXY_EXPORT void f_exception_clear(FoxyExceptionController *controller) {
    if (!controller) return;
    controller->error_count = 0;
    controller->warning_count = 0;
    controller->panic_mode = false;
    controller->had_error = false;
}