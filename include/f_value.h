#ifndef F_VALUE_H
#define F_VALUE_H

#include "f_settings.h" 
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "f_foxmode.h" // aqui vienen todos los mask / macros para trabajar con la lista maestra de bytecode.

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
typedef struct FoxyEnum FoxyEnum;

/**
 * ============================================================================
 * X-MACRO LIST: FOXY_VALUE_TYPE_LIST
 * ============================================================================
 * Define la lista maestra de tipos soportados en Foxy Runtime.
 * Mantiene la correspondencia directa entre la etiqueta enum y su nombre como cadena.
 */
#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_VALUE_TYPE_LIST(F) \
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
    F(FOXY_VAL_ARRAY,               "array")    /* [16] Arreglo dinámico en Heap */ \
    F(FOXY_VAL_DICT,                "dict")     /* [17] Tabla hash / Diccionario en Heap */ \
    F(FOXY_VAL_OBJECT,              "object")   /* [18] Instancia de clase en Heap */ \
    F(FOXY_VAL_STRUCT,              "struct")   /* [19] Estructura de datos simple en Heap */ \
    F(FOXY_VAL_CLASS,               "class")    /* [20] Metaclas de Foxy en Heap */ \
    F(FOXY_VAL_FUNCTION,            "function") /* [21] Objeto función / Closure en Heap */ \
    F(FOXY_VAL_ENUM,                "enum")     /* [22] Enumeración en Heap */

/** @brief Enumeración de tipos de datos únicos representables en la VM. */
typedef enum {
    #define F(type_enum, type_str) type_enum,
    FOXY_VALUE_TYPE_LIST(F)
    #undef F
    FOXY_VAL_COUNT
} FoxyValueType;
#else
typedef enum {
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
    FOXY_VAL_ARRAY,               /* [16] Arreglo dinámico en Heap */
    FOXY_VAL_DICT,                /* [17] Tabla hash / Diccionario en Heap */
    FOXY_VAL_OBJECT,              /* [18] Instancia de clase en Heap */
    FOXY_VAL_STRUCT,              /* [19] Estructura de datos simple en Heap */
    FOXY_VAL_CLASS,               /* [20] Metaclas de Foxy en Heap */
    FOXY_VAL_FUNCTION,            /* [21] Objeto función / Closure en Heap */
    FOXY_VAL_ENUM,                /* [22] Enumeración en Heap */
    FOXY_VAL_COUNT
} FoxyValueType;
#endif

/**
 * @brief Estructura de valor dinámico principal (Tagged Union).
 * Almacena tanto valores primitivos en Stack como referencias a objetos en Heap.
 */
typedef struct FoxyValue {
    FoxyValueType type; /**< Etiqueta única del tipo de valor */
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
        FoxyEnum *f_enum;
    } like;
} FoxyValue;

/** @brief Tabla de nombres de tipos representada en cadenas de texto (generada dinámicamente). */
extern const char * const FOXY_VALUE_TYPE_NAMES[];

/**
 * ============================================================================
 * VALORES CONSTANTES GLOBALES
 * ============================================================================
 */
#define FOXY_CONSTANT_VALUE_NULL        ((FoxyValue){ .type = FOXY_VAL_NULL })
#define FOXY_CONSTANT_VALUE_BOOL_TRUE   ((FoxyValue){ .type = FOXY_VAL_BOOL, .like.f_bool = true })
#define FOXY_CONSTANT_VALUE_BOOL_FALSE  ((FoxyValue){ .type = FOXY_VAL_BOOL, .like.f_bool = false })

/**
 * ============================================================================
 * CONSTRUCTORES DE PROTOTIPOS RÁPIDOS (COMPOUND LITERALS C99)
 * ============================================================================
 */
#define f_value_new_char(v)         ((FoxyValue){ .type = FOXY_VAL_CHAR, .like.f_char = (v) })
#define f_value_new_uchar(v)        ((FoxyValue){ .type = FOXY_VAL_UCHAR, .like.f_uchar = (v) })
#define f_value_new_short(v)        ((FoxyValue){ .type = FOXY_VAL_SHORT, .like.f_short = (v) })
#define f_value_new_ushort(v)       ((FoxyValue){ .type = FOXY_VAL_USHORT, .like.f_ushort = (v) })
#define f_value_new_int(v)          ((FoxyValue){ .type = FOXY_VAL_INT, .like.f_int = (v) })
#define f_value_new_uint(v)         ((FoxyValue){ .type = FOXY_VAL_UINT, .like.f_uint = (v) })
#define f_value_new_long(v)         ((FoxyValue){ .type = FOXY_VAL_LONG, .like.f_long = (v) })
#define f_value_new_ulong(v)        ((FoxyValue){ .type = FOXY_VAL_ULONG, .like.f_ulong = (v) })
#define f_value_new_llong(v)        ((FoxyValue){ .type = FOXY_VAL_LLONG, .like.f_llong = (v) })
#define f_value_new_ullong(v)       ((FoxyValue){ .type = FOXY_VAL_ULLONG, .like.f_ullong = (v) })
#define f_value_new_float(v)        ((FoxyValue){ .type = FOXY_VAL_FLOAT, .like.f_float = (v) })
#define f_value_new_double(v)       ((FoxyValue){ .type = FOXY_VAL_DOUBLE, .like.f_double = (v) })
#define f_value_new_ldouble(v)      ((FoxyValue){ .type = FOXY_VAL_LDOUBLE, .like.f_ldouble = (v) })
#define f_value_new_array(v)        ((FoxyValue){ .type = FOXY_VAL_ARRAY, .like.f_array = (v) })
#define f_value_new_dict(v)         ((FoxyValue){ .type = FOXY_VAL_DICT, .like.f_dict = (v) })
#define f_value_new_object(v)       ((FoxyValue){ .type = FOXY_VAL_OBJECT, .like.f_object = (v) })
#define f_value_new_struct(v)       ((FoxyValue){ .type = FOXY_VAL_STRUCT, .like.f_struct = (v) })
#define f_value_new_class(v)        ((FoxyValue){ .type = FOXY_VAL_CLASS, .like.f_class = (v) })
#define f_value_new_function(v)     ((FoxyValue){ .type = FOXY_VAL_FUNCTION, .like.f_function = (v) })
#define f_value_new_enum(v)         ((FoxyValue){ .type = FOXY_VAL_ENUM, .like.f_enum = (v) })

/** @brief Constructor auxiliar para metatipos numéricos con subtipo en runtime. */
FoxyValue f_value_new_number(double val, FoxyValueType subtype);

/**
 * ============================================================================
 * UTILIDADES Y CHECKS DE VALIDACIÓN EN RUNTIME
 * ============================================================================
 */

/** @brief Comprueba si una variable FoxyValue contiene un tipo numérico dentro del rango. */
#define f_value_is_numeric(v)       ((v).type >= (FOXY_VAL_BOOL) && (v).type <= (FOXY_VAL_NUMBER))

/** @brief Retorna la representación en texto estático del tipo de un valor. */
#define f_value_type_to_string(t)   (((t) >= 0 && (t) < (FOXY_VAL_COUNT)) ? FOXY_VALUE_TYPE_NAMES[(t)] : "unknown")

/* Operaciones de inspección de herencia y prototipos */
bool f_value_is_ancestor_of(FoxyValue parent, FoxyValue child);
bool f_value_is_descendant_of(FoxyValue child, FoxyValue parent);

/**
 * @brief Evalúa si un `FoxyValueType` es un número primitivo usando la máscara bitwise.
 * @note Operación segura de alto rendimiento libre de branch mispredictions.
 */
#define f_value_type_is_numeric(ftype) \
    (((ftype) <= FOXY_VAL_NUMBER) && (((FOXY_BIT(ftype)) & FOXY_MASK_PRIMITIVE_NUMERIC) != 0ULL))

/** @brief Evalúa si un `FoxyValueType` es de coma flotante usando la máscara bitwise. */
#define f_value_type_is_floating(ftype) \
    ((((FOXY_BIT(ftype)) & FOXY_MASK_FLOATING_POINT) != 0ULL))

/** @brief Evalúa si un `FoxyValueType` referencia un objeto dinámico en Heap. */
#define f_value_type_is_heap(ftype) \
    (((FOXY_BIT(ftype)) & FOXY_MASK_HEAP_OBJECT) != 0ULL)

/** @brief Comprueba si el struct `FoxyValue` apunta a un objeto en Heap. */
#define f_value_is_heap(v) f_value_type_is_heap((v).type)

#endif // F_VALUE_H