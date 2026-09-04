#ifndef F_FOXMODE_H
    #define F_FOXMODE_H

    #include <stdint.h>
    #include "f_foxcode.h"

    typedef uint32_t FoxInstruction;

    // Categorías principales para la clasificación primaria de tokens en la AST
    typedef enum __attribute__((__packed__)) {
        FOXY_CAT_NONE           = 0,
        FOXY_CAT_KEYWORD        = 1,
        FOXY_CAT_TYPE           = 2,
        FOXY_CAT_IDENTIFIER     = 3,
        FOXY_CAT_OPERATOR       = 4,
        FOXY_CAT_FLAGGED_METHOD = 5,
        FOXY_CAT_ERROR          = 6
    } FoxyTokenCategory;

    // Modos de parseo/ejecución
    typedef enum __attribute__((__packed__)) {
        FOXY_MODE_EXPRESSION  = 0,
        FOXY_MODE_STATEMENT   = 1,
        FOXY_MODE_DECLARATION = 2,
        FOXY_MODE_FUNCTION    = 3
    } FoxyParseMode;

    // Distribución de bits para la instrucción de 32 bits de Foxy-lang
    #define SIZE_FOX    8
    #define SIZE_A      8
    #define SIZE_B      8
    #define SIZE_C      8
    #define SIZE_BX     (SIZE_B + SIZE_C) // 16 bits para índices de constantes, saltos u offsets

    /* Desplazamientos (Shifts) */
    #define POS_FOX      0
    #define POS_A       (POS_FOX + SIZE_FOX)
    #define POS_B       (POS_A + SIZE_A)
    #define POS_C       (POS_B + SIZE_B)
    #define POS_BX      POS_B

    /* Máscaras de bits */
    #define MASK_FOX     ((1U << SIZE_FOX) - 1)
    #define MASK_A      ((1U << SIZE_A) - 1)
    #define MASK_B      ((1U << SIZE_B) - 1)
    #define MASK_C      ((1U << SIZE_C) - 1)
    #define MASK_BX     ((1U << SIZE_BX) - 1)

    /* Macros de Extracción para la VM (Decode) */
    #define GET_FOXCODE(i)  ((FOXY_FOXCODE)(((i) >> POS_FOX) & MASK_FOX))
    #define GETARG_A(i)     ((int)(((i) >> POS_A) & MASK_A))
    #define GETARG_B(i)     ((int)(((i) >> POS_B) & MASK_B))
    #define GETARG_C(i)     ((int)(((i) >> POS_C) & MASK_C))
    #define GETARG_Bx(i)    ((int)(((i) >> POS_BX) & MASK_BX))

    /* Macros de Empaquetado para el Codegen adaptadas a Foxy-lang */
    #define CREATE_ABC(fox, a, b, c) \
        ((((uint32_t)(fox)) & MASK_FOX) << POS_FOX) | \
        ((((uint32_t)(a))  & MASK_A)    << POS_A)   | \
        ((((uint32_t)(b))  & MASK_B)    << POS_B)   | \
        ((((uint32_t)(c))  & MASK_C)    << POS_C)

    #define CREATE_ABx(fox, a, bx) \
        ((((uint32_t)(fox)) & MASK_FOX) << POS_FOX) | \
        ((((uint32_t)(a))   & MASK_A)   << POS_A)   | \
        ((((uint32_t)(bx))  & MASK_BX)  << POS_BX)
#endif // F_FOXMODE_H