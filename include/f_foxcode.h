#ifndef F_FOXCODE_H
#define F_FOXCODE_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================= */
/* LISTA MAESTRA DE FOXCODES (FOXY_FOXCODE_LIST)                             */
/* Syntax: F(EnumCode)                                                       */
/* ========================================================================= */
#define FOXY_FOXCODE_LIST(F) \
    /* --------------------------------------------------------------------- */ \
    /* 1. CONTROL Y SISTEMA                                                 */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_CONTROL_NOP) \
    F(FOXCODE_CONTROL_HALT) \
    F(FOXCODE_CONTROL_INCLUDE) \
    F(FOXCODE_CONTROL_GC) \
    F(FOXCODE_EXTRAARG) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 2. CARGA DE CONSTANTES Y LITERALES                                    */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_LOAD_CONST) \
    F(FOXCODE_LOAD_NULL) \
    F(FOXCODE_LOAD_TRUE) \
    F(FOXCODE_LOAD_FALSE) \
    F(FOXCODE_LOAD_INT) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 3. CONVERSIÓN Y CASTEO DE TIPOS (NATIVO & OBJETOS)                    */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_CAST) \
    F(FOXCODE_TYPEOF) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 4. VARIABLES, SCOPES Y ENUMS                                          */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_SCOPE_LOAD_LOCAL) \
    F(FOXCODE_SCOPE_STORE_LOCAL) \
    F(FOXCODE_SCOPE_LOAD_GLOBAL) \
    F(FOXCODE_SCOPE_STORE_GLOBAL) \
    F(FOXCODE_SCOPE_LOAD_LIB) \
    F(FOXCODE_SCOPE_USE_ENUM) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 5. PROPIEDADES, MIEMBROS Y PUNTEROS STRUCT (C/POO)                    */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_GET_MEMBER) \
    F(FOXCODE_SET_MEMBER) \
    F(FOXCODE_GET_MEMBER_PTR) \
    F(FOXCODE_SET_MEMBER_PTR) \
    F(FOXCODE_ADDR_OF) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 6. INDEXACIÓN Y OPERADOR DE LONGITUD (#)                              */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_GET_INDEX) \
    F(FOXCODE_SET_INDEX) \
    F(FOXCODE_LEN) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 7. ARITMÉTICA Y COMPARACIONES                                         */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_ARITHM_ADD) \
    F(FOXCODE_ARITHM_SUB) \
    F(FOXCODE_ARITHM_MUL) \
    F(FOXCODE_ARITHM_DIV) \
    F(FOXCODE_ARITHM_MOD) \
    F(FOXCODE_ARITHM_POW) \
    F(FOXCODE_ARITHM_NEG) \
    F(FOXCODE_ARITHM_POSTINC) \
    F(FOXCODE_ARITHM_PREINC) \
    F(FOXCODE_ARITHM_POSTDEC) \
    F(FOXCODE_ARITHM_PREDEC) \
    F(FOXCODE_LOGICAL_NOT) \
    F(FOXCODE_LOGICAL_EQ) \
    F(FOXCODE_LOGICAL_NEQ) \
    F(FOXCODE_LOGICAL_LT) \
    F(FOXCODE_LOGICAL_GT) \
    F(FOXCODE_LOGICAL_LE) \
    F(FOXCODE_LOGICAL_GE) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 8. OPERADORES BITWISE                                                 */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_BITWISE_AND) \
    F(FOXCODE_BITWISE_OR) \
    F(FOXCODE_BITWISE_XOR) \
    F(FOXCODE_BITWISE_NOT) \
    F(FOXCODE_BITWISE_SHL) \
    F(FOXCODE_BITWISE_SHR) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 9. CONTROL DE FLUJO, SALTOS Y ETIQUETAS (GOTO)                        */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_FLOWCONTROL_JUMP) \
    F(FOXCODE_FLOWCONTROL_JUMP_IF_FALSE) \
    F(FOXCODE_FLOWCONTROL_JUMP_IF_TRUE) \
    F(FOXCODE_FLOWCONTROL_GOTO_LABEL) \
    F(FOXCODE_FLOWCONTROL_GOTO) \
    F(FOXCODE_FLOWCONTROL_POP) \
    F(FOXCODE_FLOWCONTROL_CALL) \
    F(FOXCODE_FLOWCONTROL_CALL_VARARG) \
    F(FOXCODE_FLOWCONTROL_RET) \
    F(FOXCODE_FLOWCONTROL_SELF_CALL) \
    F(FOXCODE_FLOWCONTROL_SUPER_CALL) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 10. ITERACIÓN Y CICLOS (FOR / FOREACH / WHILE)                        */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_LOOP_FOR_PREP) \
    F(FOXCODE_LOOP_FOR_LOOP) \
    F(FOXCODE_LOOP_ITER_PREP) \
    F(FOXCODE_LOOP_ITER_NEXT) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 11. CONSTRUCCIÓN DE ESTRUCTURAS, OBJETOS Y METATAGS                   */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_NEW_ARRAY) \
    F(FOXCODE_NEW_DICT) \
    F(FOXCODE_NEW_STRUCT) \
    F(FOXCODE_NEW_CLASS) \
    F(FOXCODE_NEW_OBJECT) \
    F(FOXCODE_METHOD_BIND) \
    F(FOXCODE_INVOKE_META) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 12. CONCURRENCIA, ENTORNOS Y ASINCRONÍA (`cjob`/`popen`)              */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_CONC_POPEN) \
    F(FOXCODE_CONC_ENV_CREATE) \
    F(FOXCODE_CONC_ENV_BIND) \
    \
    /* --------------------------------------------------------------------- */ \
    /* 13. MANEJO DE EXCEPCIONES                                             */ \
    /* --------------------------------------------------------------------- */ \
    F(FOXCODE_EXCEPTION_THROW) \
    F(FOXCODE_EXCEPTION_TRY_BEGIN) \
    F(FOXCODE_EXCEPTION_TRY_CATCH) \
    F(FOXCODE_EXCEPTION_TRY_EXCEPT) \
    F(FOXCODE_EXCEPTION_TRY_FINAL)

/* ========================================================================= */
/* GENERACIÓN AUTOMÁTICA DEL ENUM PACKED                                     */
/* ========================================================================= */
#define F(fcode) fcode,
typedef enum __attribute__((__packed__)) {
    FOXY_FOXCODE_LIST(F)
    FOXCODE_COUNT
} FOXY_FOXCODE;
#undef F

typedef FOXY_FOXCODE FoxCode;

#endif /* F_FOXCODE_H */