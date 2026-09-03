#ifndef F_VALUE_H
    #define F_VALUE_H

    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>

    typedef struct FoxyVM FoxyVM;
    typedef struct FoxyObject FoxyObject;
    typedef struct FoxyArray FoxyArray;
    typedef struct FoxyFunction FoxyFunction;
    typedef struct FoxyProtocol FoxyProtocol;
    typedef struct FoxyProcess FoxyProcess;

    // Lista maestra de tipos de datos de Foxy (añadidos string, protocol y process)
    #define FOXY_VALUE_TYPE_LIST(F) \
        F(FOXY_VAL_NULL,                "null")     \
        F(FOXY_VAL_VOID,                "void")     \
        F(FOXY_VAL_BOOL,                "bool")     \
        F(FOXY_VAL_CHAR,                "char")     \
        F(FOXY_VAL_INT,                 "int")      \
        F(FOXY_VAL_NUMBER,              "number")   \
        F(FOXY_VAL_FLOAT,               "float")    \
        F(FOXY_VAL_DOUBLE,              "double")   \
        F(FOXY_VAL_LONG,                "long")     \
        F(FOXY_VAL_LONG_LONG,           "long_long") \
        F(FOXY_VAL_UNSIGNED_LONG_LONG,  "unsigned_long_long") \
        F(FOXY_VAL_STRING,              "string")   \
        F(FOXY_VAL_ARRAY,               "array")    \
        F(FOXY_VAL_DICT,                "dict")     \
        F(FOXY_VAL_OBJECT,              "object")   \
        F(FOXY_VAL_STRUCT,              "struct")   \
        F(FOXY_VAL_CLASS,               "class")    \
        F(FOXY_VAL_FUNCTION,            "function")

    // Generación automática del enum FoxyValueType y su centinela
    typedef enum {
        #define F(type_enum, type_str) type_enum,
        FOXY_VALUE_TYPE_LIST(F)
        #undef F
        FOXY_VAL_COUNT
    } FoxyValueType;

    // Estructura de FoxyArray
    struct FoxyArray {
        uint8_t element_type_id;
        size_t length;
        void *data;
    };

    // Unión tipada segura con todos los alias requeridos por el intérprete
    typedef struct FoxyValue {
        FoxyValueType type;
        union {
            bool bval;
            bool boolean;       // Alias para bval
            char cval;
            int64_t ival;
            int64_t lval;      // Alias para ival / long
            double fval;
            double dval;       // Alias para fval / double
            const char *sval;   // Para cadenas
            const char *string; // Alias para sval
            void *ptr;
            FoxyObject *obj;
            FoxyObject *object; // Alias para obj
            FoxyArray *array;
            void *dict;
            void *klass;       // Alias para referencias a clases
            void *native_fn;
            FoxyFunction *function;
            FoxyFunction *func;  // Alias para function
            FoxyProtocol *protocol;
            FoxyProcess *process;
        } as;
    } FoxyValue;

    // Prototipos
    extern const char * const FOXY_VALUE_TYPE_STRINGS[];
    const char* f_value_type_to_string(FoxyValueType type);
    FoxyValue f_value_create_char_array(const char *str, size_t len);
    const char* f_value_get_char_array_data(const FoxyValue *val);
    void f_value_free_contents(FoxyValue *val);
#endif // F_VALUE_H