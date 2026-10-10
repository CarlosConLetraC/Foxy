#pragma once

#include "f_settings.h" 
#include "f_foxmode.h" // Aquí vienen todas las máscaras / macros para trabajar con la lista maestra de bytecode.
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * ============================================================================
 * DECLARACIONES ADELANTADAS (FORWARD DECLARATIONS)
 * ============================================================================
 * Tipos complejos alojados en memoria dinámica (Heap).
 */
typedef struct FoxyArray FoxyArray;
typedef struct FoxyDict FoxyDict;
typedef struct FoxyObject FoxyObject;
typedef struct FoxyStruct FoxyStruct;
typedef struct FoxyClass FoxyClass;
typedef struct FoxyFunction FoxyFunction;
// typedef struct FoxyEnum FoxyEnum; Los enum de foxy funcionarán como enums de C.

/**
 * ============================================================================
 * X-MACRO LIST: FOXY_VALUE_VALUE_LIST
 * ============================================================================
 * Define la lista maestra de tipos soportados en Foxy Runtime.
 * Mantiene la correspondencia directa entre la etiqueta enum y su nombre como cadena.
 */
#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_VALUE_VALUE_LIST(F) \
    F(FOXY_VAL_NULL,                "null")     /* [00] Literal nulo / Omisión */ \
    F(FOXY_VAL_BOOL,                "bool")     /* [01] Booleano (true/false) */ \
    F(FOXY_VAL_CHAR,                "char")     /* [02] Entero con signo de 8 bits */ \
    F(FOXY_VAL_UCHAR,               "uchar")    /* [03] Entero sin signo de 8 bits */ \
    F(FOXY_VAL_SHORT,               "short")    /* [04] Entero con signo de 16 bits */ \
    F(FOXY_VAL_USHORT,              "ushort")   /* [05] Entero sin signo de 16 bits */ \
    F(FOXY_VAL_INT,                 "int")      /* [06] Entero con signo nativo */ \
    F(FOXY_VAL_UINT,                "uint")     /* [07] Entero sin signo nativo */ \
    F(FOXY_VAL_LONG,                "long")     /* [08] Entero largo con signo */ \
    F(FOXY_VAL_ULONG,               "ulong")    /* [09] Entero largo sin signo */ \
    F(FOXY_VAL_LLONG,               "llong")    /* [10] Entero de 64 bits con signo */ \
    F(FOXY_VAL_ULLONG,              "ullong")   /* [11] Entero de 64 bits sin signo */ \
    F(FOXY_VAL_FLOAT,               "float")    /* [12] Coma flotante de precisión simple */ \
    F(FOXY_VAL_DOUBLE,              "double")   /* [13] Coma flotante de doble precisión */ \
    F(FOXY_VAL_LDOUBLE,             "ldouble")  /* [14] Coma flotante extendida */ \
    F(FOXY_VAL_NUMBER,              "number")   /* [15] Metatipo numérico dinámico */ \
    F(FOXY_VAL_VOID,                "void")     /* [16] Tipo de dato sin retorno */ \
    F(FOXY_VAL_ARRAY,               "array")    /* [17] Arreglo dinámico en Heap/Stack */ \
    F(FOXY_VAL_DICT,                "dict")     /* [18] Tabla hash / Diccionario en Heap */ \
    F(FOXY_VAL_OBJECT,              "object")   /* [19] Instancia de clase en Heap */ \
    F(FOXY_VAL_STRUCT,              "struct")   /* [20] Estructura de datos simple en Heap */ \
    F(FOXY_VAL_CLASS,               "class")    /* [21] Metaclas de Foxy en Heap */ \
    F(FOXY_VAL_FUNCTION,            "function") /* [22] Objeto función / Closure en Heap */ \
    F(FOXY_VAL_ENUM,                "enum")     /* [23] Enumeración en Heap */

/** @brief Enumeración de tipos de datos únicos representables en la VM. */
typedef enum FOXY_PACKED {
    #define F(type_enum, type_str) type_enum,
    FOXY_VALUE_VALUE_LIST(F)
    #undef F
} FoxyValueType;
#else
typedef enum FOXY_PACKED {
    FOXY_VAL_NULL,                /* [00] Literal nulo / Omisión */
    FOXY_VAL_BOOL,                /* [01] Booleano (true/false) */
    FOXY_VAL_CHAR,                /* [02] Entero con signo de 8 bits */
    FOXY_VAL_UCHAR,               /* [03] Entero sin signo de 8 bits */
    FOXY_VAL_SHORT,               /* [04] Entero con signo de 16 bits */
    FOXY_VAL_USHORT,              /* [05] Entero sin signo de 16 bits */
    FOXY_VAL_INT,                 /* [06] Entero con signo nativo */
    FOXY_VAL_UINT,                /* [07] Entero sin signo nativo */
    FOXY_VAL_LONG,                /* [08] Entero largo con signo */
    FOXY_VAL_ULONG,               /* [09] Entero largo sin signo */
    FOXY_VAL_LLONG,               /* [10] Entero de 64 bits con signo */
    FOXY_VAL_ULLONG,              /* [11] Entero de 64 bits sin signo */
    FOXY_VAL_FLOAT,               /* [12] Coma flotante de precisión simple */
    FOXY_VAL_DOUBLE,              /* [13] Coma flotante de doble precisión */
    FOXY_VAL_LDOUBLE,             /* [14] Coma flotante extendida */
    FOXY_VAL_NUMBER,              /* [15] Metatipo numérico dinámico */
    FOXY_VAL_VOID,                /* [16] Tipo de dato sin retorno */
    FOXY_VAL_ARRAY,               /* [17] Arreglo dinámico en Heap/Stack */
    FOXY_VAL_DICT,                /* [18] Tabla hash / Diccionario en Heap */
    FOXY_VAL_OBJECT,              /* [19] Instancia de clase en Heap */
    FOXY_VAL_STRUCT,              /* [20] Estructura de datos simple en Heap */
    FOXY_VAL_CLASS,               /* [21] Metaclas de Foxy en Heap */
    FOXY_VAL_FUNCTION,            /* [22] Objeto función / Closure en Heap */
    FOXY_VAL_ENUM                 /* [23] Enumeración en Heap */
} FoxyValueType;
#endif

/**
 * @brief Estructura de valor dinámico principal (Tagged Union).
 * Ordenada por alineación decreciente (16 bytes -> 1 byte) para eliminar -Wpadded.
 */
typedef struct FoxyValue {
    union {
        /* Primitivos numéricos y escalares */
        bool f_bool;
        signed char f_char;
        unsigned char f_uchar;
        signed short f_short;
        unsigned short f_ushort;
        signed int f_int;
        unsigned int f_uint;
        signed long f_long;
        unsigned long f_ulong;
        signed long long f_llong;
        unsigned long long f_ullong;
        float f_float;
        double f_double;
        long double f_ldouble;

        /* Punteros a estructuras en Heap */
        FoxyArray *f_array;
        FoxyDict *f_dict;
        FoxyObject *f_object;
        FoxyStruct *f_struct;
        FoxyClass *f_class;
        FoxyFunction *f_function;
    } like;                 /**< 16 bytes (requiere alineación de 16 por f_ldouble) */

    FoxyValueType type;     /**< 1 byte (FOXY_PACKED / enum) */
    uint8_t padding[15];    /**< 15 bytes explícitos para ajustar la estructura a múltiplo de 16 */
} FoxyValue;

/** @brief Tabla de nombres de tipos representada en cadenas de texto (generada dinámicamente). */
extern const char * const FOXY_VALUE_value_NAMES[];

/**
 * ============================================================================
 * VALORES CONSTANTES GLOBALES
 * ============================================================================
 */
#define FOXY_CONSTANT_VALUE_NULL        ((FoxyValue){ .type = FOXY_VAL_NULL, .padding = {0} })
#define FOXY_CONSTANT_VALUE_VOID        ((FoxyValue){ .type = FOXY_VAL_VOID, .padding = {0} })
#define FOXY_CONSTANT_VALUE_BOOL_TRUE   ((FoxyValue){ .like.f_bool = true, .type = FOXY_VAL_BOOL, .padding = {0} })
#define FOXY_CONSTANT_VALUE_BOOL_FALSE  ((FoxyValue){ .like.f_bool = false, .type = FOXY_VAL_BOOL, .padding = {0} })

/**
 * ============================================================================
 * CONSTRUCTORES DE PROTOTIPOS RÁPIDOS (MACRO-FUNCIONES f_value_new_*)
 * ============================================================================
 */
#define f_value_new_null()       (FOXY_CONSTANT_VALUE_NULL)
#define f_value_new_void()       (FOXY_CONSTANT_VALUE_VOID)
#define f_value_new_bool(v)      ((v) ? FOXY_CONSTANT_VALUE_BOOL_TRUE : FOXY_CONSTANT_VALUE_BOOL_FALSE)
#define f_value_new_char(v)      ((FoxyValue){ .like.f_char = (v), .type = FOXY_VAL_CHAR, .padding = {0} })
#define f_value_new_uchar(v)     ((FoxyValue){ .like.f_uchar = (v), .type = FOXY_VAL_UCHAR, .padding = {0} })
#define f_value_new_short(v)     ((FoxyValue){ .like.f_short = (v), .type = FOXY_VAL_SHORT, .padding = {0} })
#define f_value_new_ushort(v)    ((FoxyValue){ .like.f_ushort = (v), .type = FOXY_VAL_USHORT, .padding = {0} })
#define f_value_new_int(v)       ((FoxyValue){ .like.f_int = (v), .type = FOXY_VAL_INT, .padding = {0} })
#define f_value_new_uint(v)      ((FoxyValue){ .like.f_uint = (v), .type = FOXY_VAL_UINT, .padding = {0} })
#define f_value_new_long(v)      ((FoxyValue){ .like.f_long = (v), .type = FOXY_VAL_LONG, .padding = {0} })
#define f_value_new_ulong(v)     ((FoxyValue){ .like.f_ulong = (v), .type = FOXY_VAL_ULONG, .padding = {0} })
#define f_value_new_llong(v)     ((FoxyValue){ .like.f_llong = (v), .type = FOXY_VAL_LLONG, .padding = {0} })
#define f_value_new_ullong(v)    ((FoxyValue){ .like.f_ullong = (v), .type = FOXY_VAL_ULLONG, .padding = {0} })
#define f_value_new_float(v)     ((FoxyValue){ .like.f_float = (v), .type = FOXY_VAL_FLOAT, .padding = {0} })
#define f_value_new_double(v)    ((FoxyValue){ .like.f_double = (v), .type = FOXY_VAL_DOUBLE, .padding = {0} })
#define f_value_new_ldouble(v)   ((FoxyValue){ .like.f_ldouble = (v), .type = FOXY_VAL_LDOUBLE, .padding = {0} })
#define f_value_new_array(v)     ((FoxyValue){ .like.f_array = (v), .type = FOXY_VAL_ARRAY, .padding = {0} })
#define f_value_new_dict(v)      ((FoxyValue){ .like.f_dict = (v), .type = FOXY_VAL_DICT, .padding = {0} })
#define f_value_new_object(v)    ((FoxyValue){ .like.f_object = (v), .type = FOXY_VAL_OBJECT, .padding = {0} })
#define f_value_new_struct(v)    ((FoxyValue){ .like.f_struct = (v), .type = FOXY_VAL_STRUCT, .padding = {0} })
#define f_value_new_class(v)     ((FoxyValue){ .like.f_class = (v), .type = FOXY_VAL_CLASS, .padding = {0} })
#define f_value_new_function(v)  ((FoxyValue){ .like.f_function = (v), .type = FOXY_VAL_FUNCTION, .padding = {0} })

/**
 * ============================================================================
 * FIRMAS DE FUNCIONES f_value_*
 * ============================================================================
 */

/** @brief Constructor auxiliar para metatipos numéricos con subtipo en runtime. */
FOXY_EXPORT FoxyValue f_value_new_number(double val, FoxyValueType subtype);

/** @brief Libera los recursos asociados a un FoxyValue si corresponde. */
FOXY_EXPORT void f_value_free(FoxyValue *value);

/** @brief Compara dos FoxyValue por igualdad de tipo y contenido. */
FOXY_EXPORT bool f_value_equals(FoxyValue a, FoxyValue b);

/** @brief Imprime la representación en consola de un FoxyValue. */
FOXY_EXPORT void f_value_print(FoxyValue value);

/** @brief Operaciones de inspección de herencia y prototipos */
FOXY_EXPORT bool f_value_is_ancestor_of(FoxyValue parent, FoxyValue child);
FOXY_EXPORT bool f_value_is_descendant_of(FoxyValue child, FoxyValue parent);

/** @brief Comprueba si 'parent' es la superclase directa (padre inmediato) de 'child'. */
FOXY_EXPORT bool f_value_is_parent_of(FoxyValue parent, FoxyValue child);

/** @brief Comprueba si 'child' es una subclase/instancia directa de 'parent'. */
FOXY_EXPORT bool f_value_is_child_of(FoxyValue child, FoxyValue parent);

/** @brief Convierte cualquier FoxyValue numérico a un double (representación de precisión). */
FOXY_EXPORT double f_value_to_double(FoxyValue val);

/** @brief Convierte cualquier FoxyValue numérico a un int64_t (representación entera de 64 bits). */
FOXY_EXPORT int64_t f_value_to_int64(FoxyValue val);

/** @brief Promueve o convierte un FoxyValue numérico a otro subtipo preserving/casting su valor. */
FOXY_EXPORT FoxyValue f_value_cast_numeric(FoxyValue val, FoxyValueType target_type);

/** @brief Compara si dos FoxyValue numéricos representan el mismo valor numérico exacto aunque diferen de tipo. */
FOXY_EXPORT bool f_value_numeric_equals(FoxyValue a, FoxyValue b);

/**
 * ============================================================================
 * UTILIDADES Y CHECKS DE VALIDACIÓN EN RUNTIME (f_value_*)
 * ============================================================================
 */

/** @brief Retorna la representación en texto estático del tipo de un valor. */
#define f_value_get_type_name(t) (((t) >= 0 && (t) < (FOXY_VAL_COUNT)) ? FOXY_VALUE_value_NAMES[(t)] : "unknown")

/**
 * @brief Evalúa si un `FoxyValueType` es un número primitivo usando la máscara multipalabra.
 */
#define f_value_type_is_numeric(ftype) \
    foxy_mask_contains(FOXY_MASK_PRIMITIVE_NUMERIC, (uint32_t)(ftype))

/**
 * @brief Evalúa si un `FoxyValueType` es de coma flotante.
 */
#define f_value_type_is_floating(ftype) \
    foxy_mask_contains(FOXY_MASK_FLOATING_POINT, (uint32_t)(ftype))

/**
 * @brief Evalúa si un `FoxyValueType` referencia un objeto dinámico en Heap.
 */
#define f_value_type_is_heap(ftype) \
    foxy_mask_contains(FOXY_MASK_HEAP_OBJECT, (uint32_t)(ftype))

/** @brief Comprueba si el struct `FoxyValue` apunta a un objeto en Heap. */
#define f_value_is_heap(v) f_value_type_is_heap((v).type)