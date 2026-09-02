#ifndef F_VALUE_H
    #define F_VALUE_H

    #include "f_settings.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>

    // 1. Forward Declarations
    typedef struct FoxyObject FoxyObject;
    typedef struct FoxyClass FoxyClass;
    typedef struct FoxyFunction FoxyFunction;
    typedef struct FoxyArray FoxyArray;

    // 2. Lista de tipos unificada en Runtime
    #define FOXY_VALUE_TYPE_LIST(F) \
        F(FOXY_VAL_NULL,                "null")                \
        F(FOXY_VAL_CHAR,                "char")                \
        F(FOXY_VAL_INT,                 "int")                 \
        F(FOXY_VAL_NUMBER,              "number")              \
        F(FOXY_VAL_FLOAT,               "float")               \
        F(FOXY_VAL_DOUBLE,              "double")              \
        F(FOXY_VAL_LONG,                "long")                \
        F(FOXY_VAL_LONG_LONG,           "long long")           \
        F(FOXY_VAL_UNSIGNED_LONG_LONG,  "unsigned long long")  \
        F(FOXY_VAL_BOOL,                "bool")                \
        F(FOXY_VAL_ARRAY,               "array")               \
        F(FOXY_VAL_OBJECT,              "object")              \
        F(FOXY_VAL_FUNCTION,            "function")            \
        F(FOXY_VAL_CLASS,               "class")               \
        F(FOXY_VAL_DICT,                "dict")                \
        F(FOXY_VAL_STRUCT,              "struct")

    #define F(type_enum, type_str) type_enum,
    typedef enum __attribute__((__packed__)) {
        FOXY_VALUE_TYPE_LIST(F)
        FOXY_VAL_COUNT
    } FoxyValueType;
    #undef F

    typedef enum __attribute__((__packed__)) {
        FOXY_TYPE_PRIMITIVE,
        FOXY_TYPE_OBJECT
    } FoxyTypeKind;

    // 3. Estructura Base del Objeto en el Heap (Encabezado común para GC)
    struct FoxyObject {
        uint32_t ref_count;      // Control de referencias
        uint8_t  marked;         // Flag para Garbage Collector
        FoxyClass *klass;        // Puntero a la Metaclase/Clase
        struct FoxyField *fields;// Atributos (si aplican a nivel general o de instancia)
        size_t field_count;
        size_t field_capacity;
    };
    
    // 4. Estructura de un Array genérico en el Heap
    struct FoxyArray {
        FoxyObject header;        /* Encabezado común de objeto (ref count, GC, etc.) */
        uint16_t element_type_id; /* ID del tipo (ej: FOXY_T_INT, FOXY_T_FLOAT, o ID de Class) */
        uint8_t  element_kind;    /* FOXY_TYPE_PRIMITIVE vs FOXY_TYPE_OBJECT */
        size_t   length;          /* Cantidad de elementos */
        size_t   capacity;        /* Capacidad reservada */
        void*    data;            /* Puntero al bloque contiguo de datos */
    };

    // 5. Estructura contenedora de valores en ejecución (Tagged Union)
    typedef struct FoxyValue {
        FoxyValueType type;
        union {
            bool boolean;
            bool bval;           // Alias para booleans
            char   cval;         // <--- Alias para caracteres (FOXY_VAL_CHAR)
            double number;
            double dval;         // Alias para doubles
            double numval;       // <--- Alias solicitado para números
            float  fval;         // <--- Alias solicitado para floats
            int32_t ival;        // Para enteros de 32 bits
            int64_t lval;        // <--- Alias solicitado para longs
            long long llval;     // <--- Alias solicitado para long longs
            unsigned long long ullval; // <--- Alias solicitado para unsigned long longs
            char *sval;
            char *string;
            struct FoxyDict *dict;
            struct FoxyObject *object;
            void *obj;
            FoxyArray *array;
            struct FoxyFunction *func; // <--- Alias solicitado para funciones
            struct FoxyClass *klass;   // <--- Alias solicitado para clases
            void *native_fn;
        } as;
    } FoxyValue;
    #define FOXY_NULL_VALUE ((FoxyValue){ .type = FOXY_VAL_NULL, .as.object = NULL })
    
    const char* f_value_type_to_string(FoxyValueType type);
#endif // F_VALUE_H