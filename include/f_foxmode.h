#ifndef F_FOXMODE_H
#define F_FOXMODE_H

#include <stdint.h>
#include "f_settings.h"
#include "f_value.h"

/**
 * ============================================================================
 * FOXY BYTECODE ARCHITECTURE (FOX_INST / FOX_OP)
 * ============================================================================
 * 
 * Todas las instrucciones de Foxy Bytecode son palabras fijas de 32 bits (uint32_t)
 * alineadas en memoria, diseñadas para un modelo de Máquina Virtual basada en Registros.
 * 
 * ----------------------------------------------------------------------------
 * LAYOUT DE MEMORIA A NIVEL DE BITS (32 BITS)
 * ----------------------------------------------------------------------------
 * Los 20 bits superiores (Opcode, Registro Destino A, y Banderas) permanecen en
 * posiciones strictly FIJAS en todos los formatos. Esto permite al ciclo de
 * despacho (dispatch loop) de la VM extraer Opcode, A y Flags en una sola pasada
 * sin importar la variante de la instrucción.
 * 
 * FORMATO 1: iABC (Operaciones Aritméticas, Comparaciones y Lógica R-R / R-R-R)
 * 31       24 23       16 15   12 11    8 7          0
 * +----------+----------+--------+-------+-----------+
 * |  Opcode  |    A     | Flags  |   B   |     C     |
 * |  (8 bits)| (8 bits) |(4 bits)|(4b/r) | (8 bits)  |
 * +----------+----------+--------+-------+-----------+
 * 
 * FORMATO 2: iABx (Cargas de Constantes, Literales, Ámbitos Globales/Upvalues)
 * 31       24 23       16 15   12 11                 0
 * +----------+----------+--------+-------------------+
 * |  Opcode  |    A     | Flags  |        Bx         |
 * |  (8 bits)| (8 bits) |(4 bits)| (12 bits Unsigned)|
 * +----------+----------+--------+-------------------+
 * 
 * FORMATO 3: iAsBx (Saltos Condicionales / Incondicionales con Offset)
 * 31       24 23       16 15   12 11                 0
 * +----------+----------+--------+-------------------+
 * |  Opcode  |    A     | Flags  |        sBx        |
 * |  (8 bits)| (8 bits) |(4 bits)|  (12 bits Signed) |
 * +----------+----------+--------+-------------------+
 * 
 * ----------------------------------------------------------------------------
 * DESGLOSE DE CAMPOS
 * ----------------------------------------------------------------------------
 * - Opcode (8 bits)  : [0x00 - 0xFF] Código de operación (hasta 256 instrucciones).
 * - A      (8 bits)  : [r0 - r255] Registro destino o registro primario de evaluación.
 * - Flags  (4 bits)  : [0x0 - 0xF] Modificadores de ejecución activa (Modo Híbrido).
 * - B      (4 bits)  : [r0 - r15]  Registro fuente secundario (Formato iABC).
 * - C      (8 bits)  : [r0 - r255] Registro fuente terciario o inmediato corto.
 * - Bx     (12 bits) : [0 - 4095]  Índice directo a la tabla de constantes o símbolos.
 * - sBx    (12 bits) : [-2047 a +2047] Desplazamiento de salto relativo con sesgo de 2047.
 * ============================================================================
 */

/** @brief Tipo de dato nativo para representar una instrucción de bytecode de 32 bits. */
typedef uint32_t FoxyInstruction;

#if FOXY_COMPILER_SUPPORTS_XMACROS

    /**
     * @brief Lista X-Macro de banderas de instrucción (Flags de 4 bits).
     * Modifican el comportamiento semántico del dispatch sin alterar la opcode base.
     */
    #define FOXY_INSTRUCTION_FLAG_LIST(F) \
        F(FOX_INST_FLAG_NONE,        0x0u)      /* [0x0] Sin restricciones (dinámico puro) */ \
        F(FOX_INST_FLAG_CONSTRAINED, (1u << 0)) /* [Bit 0] Valida constraint_type en runtime */ \
        F(FOX_INST_FLAG_READONLY,    (1u << 1)) /* [Bit 1] Protege contra reasignación (Inmutable/Const) */ \
        F(FOX_INST_FLAG_AUTO_CAST,   (1u << 2)) /* [Bit 2] Aplica coerción implícita si aplica */ \
        F(FOX_INST_FLAG_UNGUARDED,   (1u << 3)) /* [Bit 3] Omite verificaciones en caliente (JIT/Fast-path) */

    /** @brief Enumeración generada automáticamente desde la X-Macro de flags. */
    typedef enum {
        #define F(flag_enum, flag_val) flag_enum = flag_val,
        FOXY_INSTRUCTION_FLAG_LIST(F)
        #undef F
    } FoxyInstructionFlags;

#else // Fallback manual para compiladores limitados

    typedef enum {
        FOX_INST_FLAG_NONE        = 0x0u,      /* Sin restricciones (dinámico puro) */
        FOX_INST_FLAG_CONSTRAINED = (1u << 0), /* Bit 0: Valida constraint_type en runtime */
        FOX_INST_FLAG_READONLY    = (1u << 1), /* Bit 1: Protege contra reasignación (Inmutable) */
        FOX_INST_FLAG_AUTO_CAST   = (1u << 2), /* Bit 2: Aplica coerción implícita si aplica */
        FOX_INST_FLAG_UNGUARDED   = (1u << 3)  /* Bit 3: Omite verificaciones en caliente (JIT/Fast-path) */
    } FoxyInstructionFlags;

#endif

/**
 * ============================================================================
 * CONSTANTES DE POSICIONAMIENTO Y MÁSCARAS DE BITS (BYTECODE LAYOUT)
 * ============================================================================
 */

/* Desplazamientos de bits (Shifts) para empaquetado y desempaquetado */
#define FOXMODE_SHIFT_OPCODE           24u  /**< @brief Opcode ocupa bits [31:24] (8 bits) */
#define FOXMODE_SHIFT_A                16u  /**< @brief Registro A ocupa bits [23:16] (8 bits) */
#define FOXMODE_SHIFT_FLAGS            12u  /**< @brief Flags ocupan bits [15:12] (4 bits) */
#define FOXMODE_SHIFT_B                8u   /**< @brief Registro B ocupa bits [11:8] (4 bits) */

/* Máscaras de aislamiento de bits (Bitwise Masks) */
#define FOXMODE_MASK_8BIT              0xFFu   /**< @brief Mantiene valores de 8 bits (0 a 255) */
#define FOXMODE_MASK_4BIT              0x0Fu   /**< @brief Mantiene valores de 4 bits (0 a 15) */
#define FOXMODE_MASK_12BIT             0x0FFFu /**< @brief Mantiene valores de 12 bits (0 a 4095) */

/* Sesgo para codificación de saltos relativos con signo (Signed Offset) */
#define FOXMODE_SBX_BIAS               2047    /**< @brief Mapea rango [-2047, +2047] a [0, 4094] */

/**
 * ============================================================================
 * EXTRACCIÓN DE CAMPOS EN TIEMPO DE DESPACHO (DISPATCH HELPERS)
 * ============================================================================
 */

/** @brief Extrae el Opcode (8 bits superiores: 31-24) de la instrucción. */
#define FOXMODE_GET_OPCODE(i)   ((uint8_t)(((i) >> FOXMODE_SHIFT_OPCODE) & FOXMODE_MASK_8BIT))

/** @brief Extrae el registro destino A (8 bits: 23-16) de la instrucción. */
#define FOXMODE_GET_A(i)        ((uint8_t)(((i) >> FOXMODE_SHIFT_A) & FOXMODE_MASK_8BIT))

/** @brief Extrae las banderas de ejecución (4 bits: 15-12) de la instrucción. */
#define FOXMODE_GET_FLAGS(i)    ((uint8_t)(((i) >> FOXMODE_SHIFT_FLAGS) & FOXMODE_MASK_4BIT))

/** @brief Extrae el registro fuente B (4 bits: 11-8) en formato iABC. */
#define FOXMODE_GET_B_4B(i)     ((uint8_t)(((i) >> FOXMODE_SHIFT_B) & FOXMODE_MASK_4BIT))

/** @brief Extrae el registro fuente C o inmediato corto (8 bits: 7-0) en formato iABC. */
#define FOXMODE_GET_C(i)        ((uint8_t)((i) & FOXMODE_MASK_8BIT))

/** @brief Extrae el operando sin signo Bx de 12 bits (11-0) en formato iABx. */
#define FOXMODE_GET_BX(i)       ((uint16_t)((i) & FOXMODE_MASK_12BIT))

/** @brief Extrae el desplazamiento con signo sBx de 12 bits (11-0) restando el sesgo SBX_BIAS. */
#define FOXMODE_GET_SBX(i)      ((int16_t)((int32_t)((i) & FOXMODE_MASK_12BIT) - FOXMODE_SBX_BIAS))

/**
 * ============================================================================
 * EMISIÓN / EMPAQUETADO (CODEGEN HELPERS)
 * ============================================================================
 */

/**
 * @brief Crea una instrucción en formato iABC (R-R / R-R-R).
 * @param op Opcode (8 bits)
 * @param a Registro destino A (8 bits)
 * @param flags Banderas de ejecución (4 bits)
 * @param b Registro fuente B (4 bits)
 * @param c Registro fuente C o inmediato corto (8 bits)
 */
#define FOXMODE_CREATE_iABC(op, a, flags, b, c) \
    (((uint32_t)(op)    << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)     << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     (((uint32_t)(b)     & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_B) | \
     ((uint32_t)(c)      & FOXMODE_MASK_8BIT))

/**
 * @brief Crea una instrucción en formato iABx (Índices a tablas de constantes/upvalues).
 * @param op Opcode (8 bits)
 * @param a Registro destino A (8 bits)
 * @param flags Banderas de ejecución (4 bits)
 * @param bx Índice o inmediato de 12 bits sin signo (0 a 4095)
 */
#define FOXMODE_CREATE_iABx(op, a, flags, bx) \
    (((uint32_t)(op)     << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)      << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     ((uint32_t)(bx)     & FOXMODE_MASK_12BIT))

/**
 * @brief Crea una instrucción en formato iAsBx (Saltos condicionales/incondicionales).
 * @param op Opcode (8 bits)
 * @param a Registro destino o evaluador A (8 bits)
 * @param flags Banderas de ejecución (4 bits)
 * @param sbx Desplazamiento relativo de salto con signo (-2047 a +2047)
 */
#define FOXMODE_CREATE_iAsBx(op, a, flags, sbx) \
    (((uint32_t)(op)    << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)     << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     ((uint32_t)((int32_t)(sbx) + FOXMODE_SBX_BIAS) & FOXMODE_MASK_12BIT))

/**
 * ============================================================================
 * MÁSCARAS BITWISE PARA INSPECCIÓN RÁPIDA DE TIPOS (TYPE CHECKING)
 * ============================================================================
 */

/** @brief Genera una máscara de bit de 64 bits para la posición especificada por f_type. */
#define FOXY_BIT(f_type) (1ULL << (f_type))

/** @brief Máscara conteniendo todos los tipos numéricos y primitivos acelerados. */
#define FOXY_MASK_PRIMITIVE_NUMERIC \
    (FOXY_BIT(FOXY_VAL_BOOL)   | FOXY_BIT(FOXY_VAL_CHAR)    | FOXY_BIT(FOXY_VAL_UCHAR)  | \
     FOXY_BIT(FOXY_VAL_SHORT)  | FOXY_BIT(FOXY_VAL_USHORT)  | FOXY_BIT(FOXY_VAL_INT)    | \
     FOXY_BIT(FOXY_VAL_UINT)   | FOXY_BIT(FOXY_VAL_LONG)    | FOXY_BIT(FOXY_VAL_ULONG)  | \
     FOXY_BIT(FOXY_VAL_LLONG)  | FOXY_BIT(FOXY_VAL_ULLONG)  | FOXY_BIT(FOXY_VAL_FLOAT)  | \
     FOXY_BIT(FOXY_VAL_DOUBLE) | FOXY_BIT(FOXY_VAL_LDOUBLE))

/** @brief Máscara de aislamiento para tipos de coma flotante. */
#define FOXY_MASK_FLOATING_POINT \
    (FOXY_BIT(FOXY_VAL_FLOAT) | FOXY_BIT(FOXY_VAL_DOUBLE) | FOXY_BIT(FOXY_VAL_LDOUBLE))

/** @brief Máscara de aislamiento para enteros sin signo (Unsigned Integers). */
#define FOXY_MASK_UNSIGNED_INT \
    (FOXY_BIT(FOXY_VAL_UCHAR) | FOXY_BIT(FOXY_VAL_USHORT) | FOXY_BIT(FOXY_VAL_UINT) | \
     FOXY_BIT(FOXY_VAL_ULONG) | FOXY_BIT(FOXY_VAL_ULLONG))

/** @brief Máscara para identificar estructuras complejas almacenadas en el Heap (Punteros). */
#define FOXY_MASK_HEAP_OBJECT \
    (FOXY_BIT(FOXY_VAL_ARRAY)    | FOXY_BIT(FOXY_VAL_DICT)     | \
     FOXY_BIT(FOXY_VAL_OBJECT)   | FOXY_BIT(FOXY_VAL_STRUCT)   | \
     FOXY_BIT(FOXY_VAL_CLASS)    | FOXY_BIT(FOXY_VAL_FUNCTION) | \
     FOXY_BIT(FOXY_VAL_ENUM))

#endif // F_FOXMODE_H