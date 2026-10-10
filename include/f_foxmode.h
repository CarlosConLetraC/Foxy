#pragma once

#include "f_settings.h"
#include "f_value.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * ============================================================================
 * FOXY BYTECODE ARCHITECTURE (FOX_INST / FOX_OP): Tabla Maestra de OpCode
 * ============================================================================
 */

typedef uint32_t FoxyInstruction;

#if FOXY_COMPILER_SUPPORTS_XMACROS

    #define FOXY_INSTRUCTION_FLAG_LIST(F) \
        F(FOX_INST_FLAG_NONE,        0x0u)      \
        F(FOX_INST_FLAG_CONSTRAINED, (1u << 0)) \
        F(FOX_INST_FLAG_READONLY,    (1u << 1)) \
        F(FOX_INST_FLAG_AUTO_CAST,   (1u << 2)) \
        F(FOX_INST_FLAG_UNGUARDED,   (1u << 3))

    typedef enum FOXY_PACKED {
        #define F(flag_enum, flag_val) flag_enum = flag_val,
        FOXY_INSTRUCTION_FLAG_LIST(F)
        #undef F
    } FoxyInstructionFlags;

#else

    typedef enum FOXY_PACKED {
        FOX_INST_FLAG_NONE        = 0x0u,
        FOX_INST_FLAG_CONSTRAINED = (1u << 0),
        FOX_INST_FLAG_READONLY    = (1u << 1),
        FOX_INST_FLAG_AUTO_CAST   = (1u << 2),
        FOX_INST_FLAG_UNGUARDED   = (1u << 3)
    } FoxyInstructionFlags;

#endif

/* Desplazamientos de bits para empaquetado y desempaquetado */
#define FOXMODE_SHIFT_OPCODE           24u
#define FOXMODE_SHIFT_A                16u
#define FOXMODE_SHIFT_FLAGS            12u
#define FOXMODE_SHIFT_B                8u

/* Máscaras de aislamiento de bits */
#define FOXMODE_MASK_8BIT              0xFFu
#define FOXMODE_MASK_4BIT              0x0Fu
#define FOXMODE_MASK_12BIT             0x0FFFu

/* Sesgo para codificación de saltos relativos con signo */
#define FOXMODE_SBX_BIAS               2047

static inline uint16_t f_foxmode_compose_type(uint16_t category, uint16_t subtype) {
    return (uint16_t)(((category & FOXMODE_MASK_4BIT) << 12) | (subtype & FOXMODE_MASK_12BIT));
}

#define FOXMODE_GET_OPCODE(i)   ((uint8_t)(((uint32_t)(i) >> FOXMODE_SHIFT_OPCODE) & FOXMODE_MASK_8BIT))
#define FOXMODE_GET_A(i)        ((uint8_t)(((uint32_t)(i) >> FOXMODE_SHIFT_A) & FOXMODE_MASK_8BIT))
#define FOXMODE_GET_FLAGS(i)    ((uint8_t)(((uint32_t)(i) >> FOXMODE_SHIFT_FLAGS) & FOXMODE_MASK_4BIT))
#define FOXMODE_GET_B_4B(i)     ((uint8_t)(((uint32_t)(i) >> FOXMODE_SHIFT_B) & FOXMODE_MASK_4BIT))
#define FOXMODE_GET_C(i)        ((uint8_t)((uint32_t)(i) & FOXMODE_MASK_8BIT))
#define FOXMODE_GET_BX(i)       ((uint16_t)((uint32_t)(i) & FOXMODE_MASK_12BIT))

static inline int16_t f_foxmode_get_sbx(uint32_t i) {
    int32_t raw_bx = (int32_t)(i & FOXMODE_MASK_12BIT);
    return (int16_t)(raw_bx - FOXMODE_SBX_BIAS);
}

#define FOXMODE_GET_SBX(i) f_foxmode_get_sbx((uint32_t)(i))

#define FOXMODE_CREATE_iABC(op, a, flags, b, c) \
    (((uint32_t)(op)    << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)     << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     (((uint32_t)(b)     & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_B) | \
     ((uint32_t)(c)      & FOXMODE_MASK_8BIT))

#define FOXMODE_CREATE_iABx(op, a, flags, bx) \
    (((uint32_t)(op)     << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)      << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     ((uint32_t)(bx)     & FOXMODE_MASK_12BIT))

static inline uint32_t f_foxmode_create_iAsBx(uint8_t op, uint8_t a, uint8_t flags, int16_t sbx) {
    int32_t biased_sbx = (int32_t)sbx + FOXMODE_SBX_BIAS;
    if (biased_sbx < 0) biased_sbx = 0;
    if (biased_sbx > (int32_t)FOXMODE_MASK_12BIT) biased_sbx = (int32_t)FOXMODE_MASK_12BIT;

    return (((uint32_t)op << FOXMODE_SHIFT_OPCODE) |
            ((uint32_t)a  << FOXMODE_SHIFT_A) |
            (((uint32_t)flags & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) |
            ((uint32_t)biased_sbx & FOXMODE_MASK_12BIT));
}

#define FOXMODE_CREATE_iAsBx(op, a, flags, sbx) \
    f_foxmode_create_iAsBx((uint8_t)(op), (uint8_t)(a), (uint8_t)(flags), (int16_t)(sbx))

/**
 * ============================================================================
 * INFRAESTRUCTURA DE MÁSCARAS BITWISE MULTIPALABRA (HASTA 256+ TOKENS/KINDS)
 * ============================================================================
 */

/* Rango total de elementos soportados por la máscara maestra (por defecto 256) */
#define FOXY_MASTER_MASK_SIZE_BITS     256U
#define FOXY_MASK_WORD_BITS            64U
#define FOXY_MASK_WORDS                ((FOXY_MASTER_MASK_SIZE_BITS + FOXY_MASK_WORD_BITS - 1U) / FOXY_MASK_WORD_BITS)

/**
 * @brief Estructura de máscara maestra basada en bloques de 64 bits.
 */
typedef struct {
    uint64_t words[FOXY_MASK_WORDS];
} foxy_mask_t;

/* Macros de cálculo de bloque (word offset) y bit offset dentro del bloque */
#define FOXY_MASK_WORD_INDEX(id)       ((uint32_t)(id) / FOXY_MASK_WORD_BITS)
#define FOXY_MASK_BIT_SHIFT(id)        ((uint32_t)(id) % FOXY_MASK_WORD_BITS)
#define FOXY_MASK_BIT_VALUE(id)        (1ULL << FOXY_MASK_BIT_SHIFT(id))

/**
 * @brief Genera una máscara de bit de 64 bits acotada para FoxyValueType en el primer bloque (Word 0).
 */
/* Macros auxiliares para calcular el bit relativo dentro del bloque de 64 bits */
#define FOXY_TOK_BIT_W0(tok) (1ULL << (((uint64_t)(tok) - (64ULL * 0ULL)/*0ULL*/) & 63U))
#define FOXY_TOK_BIT_W1(tok) (1ULL << (((uint64_t)(tok) - (64ULL * 1ULL)/*64ULL*/) & 63U))
#define FOXY_TOK_BIT_W2(tok) (1ULL << (((uint64_t)(tok) - (64ULL * 2ULL)/*128ULL*/) & 63U))
#define FOXY_TOK_BIT_W3(tok) (1ULL << (((uint64_t)(tok) - (64ULL * 3ULL)/*192ULL*/) & 63U))
#define FOXY_VAL_BIT(f_type)           (1ULL << ((uint64_t)(f_type) & 63U))

#define FOXY_BIT128(tok) FOXY_TOK_BIT_W0(tok)

/**
 * @brief Constructor literal para inicialización estática de foxy_mask_t (4 palabras de 64 bits)
 */
#define FOXY_MAKE_MASK(w0, w1, w2, w3) \
    ((foxy_mask_t){ .words = { (uint64_t)(w0), (uint64_t)(w1), (uint64_t)(w2), (uint64_t)(w3) } })

/**
 * @brief Evalúa en O(1) libre de desbordamiento si un ID está presente en la máscara.
 */
static inline bool foxy_mask_contains(foxy_mask_t mask, uint32_t id) {
    if (id >= FOXY_MASTER_MASK_SIZE_BITS) return false;
    uint32_t word_idx = FOXY_MASK_WORD_INDEX(id);
    return (mask.words[word_idx] & FOXY_MASK_BIT_VALUE(id)) != 0ULL;
}

/**
 * @brief Realiza la unión (OR bitwise) entre dos máscaras maestras.
 */
static inline foxy_mask_t foxy_mask_or(foxy_mask_t a, foxy_mask_t b) {
    foxy_mask_t res;
    for (size_t i = 0; i < FOXY_MASK_WORDS; ++i) {
        res.words[i] = a.words[i] | b.words[i];
    }
    return res;
}

/**
 * ============================================================================
 * MÁSCARAS BITWISE PARA FoxyValueType (EN MÁSCARA MAESTRA foxy_mask_t)
 * ============================================================================
 */

/** @brief Máscara conteniendo todos los tipos numéricos y primitivos acelerados. */
#define FOXY_MASK_PRIMITIVE_NUMERIC FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    (FOXY_VAL_BIT(FOXY_VAL_BOOL)   | \
     FOXY_VAL_BIT(FOXY_VAL_CHAR)   | \
     FOXY_VAL_BIT(FOXY_VAL_UCHAR)  | \
     FOXY_VAL_BIT(FOXY_VAL_SHORT)  | \
     FOXY_VAL_BIT(FOXY_VAL_USHORT) | \
     FOXY_VAL_BIT(FOXY_VAL_INT)    | \
     FOXY_VAL_BIT(FOXY_VAL_UINT)   | \
     FOXY_VAL_BIT(FOXY_VAL_LONG)   | \
     FOXY_VAL_BIT(FOXY_VAL_ULONG)  | \
     FOXY_VAL_BIT(FOXY_VAL_LLONG)  | \
     FOXY_VAL_BIT(FOXY_VAL_ULLONG) | \
     FOXY_VAL_BIT(FOXY_VAL_FLOAT)  | \
     FOXY_VAL_BIT(FOXY_VAL_DOUBLE) | \
     FOXY_VAL_BIT(FOXY_VAL_LDOUBLE)), \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/** @brief Máscara de aislamiento para tipos de coma flotante. */
#define FOXY_MASK_FLOATING_POINT FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    (FOXY_VAL_BIT(FOXY_VAL_FLOAT)  | \
     FOXY_VAL_BIT(FOXY_VAL_DOUBLE) | \
     FOXY_VAL_BIT(FOXY_VAL_LDOUBLE)), \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/** @brief Máscara de aislamiento para enteros sin signo (Unsigned Integers). */
#define FOXY_MASK_UNSIGNED_INT FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    (FOXY_VAL_BIT(FOXY_VAL_UCHAR)  | \
     FOXY_VAL_BIT(FOXY_VAL_USHORT) | \
     FOXY_VAL_BIT(FOXY_VAL_UINT)   | \
     FOXY_VAL_BIT(FOXY_VAL_ULONG)  | \
     FOXY_VAL_BIT(FOXY_VAL_ULLONG)), \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/** @brief Máscara para identificar estructuras complejas almacenadas en el Heap (Punteros). */
#define FOXY_MASK_HEAP_OBJECT FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    (FOXY_VAL_BIT(FOXY_VAL_ARRAY)    | \
     FOXY_VAL_BIT(FOXY_VAL_DICT)     | \
     FOXY_VAL_BIT(FOXY_VAL_OBJECT)   | \
     FOXY_VAL_BIT(FOXY_VAL_STRUCT)   | \
     FOXY_VAL_BIT(FOXY_VAL_CLASS)    | \
     FOXY_VAL_BIT(FOXY_VAL_FUNCTION) | \
     FOXY_VAL_BIT(FOXY_VAL_ENUM)), \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

