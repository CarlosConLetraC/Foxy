#ifndef F_STATUS_H
#define F_STATUS_H

#define FOXY_STATUS_LIST(F) \
    F(FOXY_STATUS_OK,                    "Operación completada con éxito") \
    /* Errores del Sistema / Memory */ \
    F(FOXY_STATUS_ERROR_OUT_OF_MEMORY,   "Error de asignación de memoria (OOM)") \
    F(FOXY_STATUS_ERROR_IO,              "Error de entrada/salida de archivos") \
    /* Errores de Análisis Léxico y Sintáctico */ \
    F(FOXY_STATUS_ERROR_LEXICAL,         "Error léxico en tiempo de escaneo") \
    F(FOXY_STATUS_ERROR_SYNTAX,          "Error sintáctico durante el parsing") \
    /* Errores de Compilación / Tipado */ \
    F(FOXY_STATUS_ERROR_COMPILE,         "Error de compilación de bytecode") \
    F(FOXY_STATUS_ERROR_TYPE_MISMATCH,   "Incompatibilidad de tipos en tiempo de compilación") \
    F(FOXY_STATUS_ERROR_UNDECLARED_VAR,  "Identificador o variable no declarada") \
    F(FOXY_STATUS_ERROR_REDECLARED_VAR,  "Redeclaración de símbolo en el mismo ámbito") \
    /* Errores de Runtime / VM */ \
    F(FOXY_STATUS_ERROR_RUNTIME,          "Error de ejecución en la VM") \
    F(FOXY_STATUS_ERROR_STACK_OVERFLOW,   "Desbordamiento de la pila de llamadas (Stack Overflow)") \
    F(FOXY_STATUS_ERROR_STACK_UNDERFLOW,  "Subdesbordamiento de la pila de evaluación") \
    F(FOXY_STATUS_ERROR_DIVISION_BY_ZERO, "División o módulo por cero") \
    F(FOXY_STATUS_ERROR_NULL_POINTER,     "Desreferencia de puntero/instancia nula (NullPointer)") \
    F(FOXY_STATUS_ERROR_INDEX_OUT_BOUNDS, "Índice fuera de los límites del contenedor") \
    F(FOXY_STATUS_ERROR_OUT_OF_BOUNDS,    "Valor o casting fuera de rango numérico") \
    F(FOXY_STATUS_ERROR_ASSERTION_FAILED, "Fallo en verificación de aserción (Assertion Failed)")

typedef enum {
    #define F(code, str) code,
    FOXY_STATUS_LIST(F)
    FOXY_STATUS_COUNT
    #undef F
} FoxyStatus;

/***
 * Convierte un código FoxyStatus a su mensaje descriptivo.
 ***/
const char* f_status_to_string(FoxyStatus status);
#endif /* F_STATUS_H */