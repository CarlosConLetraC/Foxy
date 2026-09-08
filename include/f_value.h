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

    #define FOXY_VALUE_TYPE_LIST(F) \
        F(FOXY_VAL_NULL,                "null")     /*[0]*/\
        F(FOXY_VAL_VOID,                "void")     /*[1]*/\
        F(FOXY_VAL_BOOL,                "bool")     /*[2]*/\
        F(FOXY_VAL_CHAR,                "char")     /*[3]*/\
        F(FOXY_VAL_INT,                 "int")      /*[4]*/\
        F(FOXY_VAL_NUMBER,              "number")   /*[5]*/\
        F(FOXY_VAL_FLOAT,               "float")    /*[6]*/\
        F(FOXY_VAL_DOUBLE,              "double")   /*[7]*/\
        F(FOXY_VAL_LONG,                "long")     /*[8]*/\
        F(FOXY_VAL_LONG_LONG,           "llong")    /*[9]*/\
        F(FOXY_VAL_UNSIGNED_LONG_LONG,  "ullong")   /*[10]*/\
        F(FOXY_VAL_ARRAY,               "array")    /*[11]*/\
        F(FOXY_VAL_DICT,                "dict")     /*[12]*/\
        F(FOXY_VAL_OBJECT,              "object")   /*[13]*/\
        F(FOXY_VAL_STRUCT,              "struct")   /*[14]*/\
        F(FOXY_VAL_CLASS,               "class")    /*[15]*/\
        F(FOXY_VAL_FUNCTION,            "function") /*[16]*/

    typedef enum {
        #define F(type_enum, type_str) type_enum,
        FOXY_VALUE_TYPE_LIST(F)
        #undef F
        FOXY_VAL_COUNT
    } FoxyValueType;

    typedef struct FoxyValue {
        FoxyValueType type;
        union {
            bool bval;
            bool boolean;
            char cval;
            int64_t ival;
            int64_t lval;
            double fval;
            double dval;
            const char *sval;
            const char *string;
            void *ptr;
            FoxyObject *obj;
            FoxyArray *array;
            void *dict;
            void *klass;
            void *native_fn;
            FoxyFunction *func;
            FoxyProtocol *protocol;
            FoxyProcess *process;
        } as;
    } FoxyValue;

    extern const char * const FOXY_VALUE_TYPE_NAMES[];
    const char* f_value_type_to_char_array(FoxyValueType type);
    FoxyValue f_value_create_char_array(const char *str, size_t len);
    const char* f_value_get_char_array_data(const FoxyValue *val);
    void f_value_free_contents(FoxyValue *val);

    bool f_value_is_numeric(const FoxyValue *val);
    bool f_value_is_pure_integer(const FoxyValue *val);
    double f_value_as_double(const FoxyValue *val);
    bool f_value_equals(const FoxyValue *a, const FoxyValue *b);
#endif // F_VALUE_H