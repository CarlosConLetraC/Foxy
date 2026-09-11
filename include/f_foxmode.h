#ifndef F_FOXMODE_H
#define F_FOXMODE_H

#include <stdint.h>
#include "f_settings.h"

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
 * posiciones estrictamente FIJAS en todos los formatos. Esto permite al ciclo de
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
 * 
 * ----------------------------------------------------------------------------
 * REPRESENTACIÓN TIPO ASSEMBLY Y TRADUCCIÓN A MACROS
 * ----------------------------------------------------------------------------
 * 1. Declaración Dinámica (Sin restricciones):
 *    Assembly : MOV r0, r1
 *    C Macro  : FOXMODE_CREATE_iABC(OP_MOV, 0, 1, FOX_INST_FLAG_NONE, 0)
 *    Bytecode : [ OP_MOV | r0 | 0000 | r1 | 00000000 ]
 * 
 * 2. Declaración Tipada / Restringida (int foo = 5):
 *    Assembly : SET_VAR r0, #42 {CONSTRAINED}
 *    C Macro  : FOXMODE_CREATE_iABx(OP_SET_VAR, 0, FOX_INST_FLAG_CONSTRAINED, 42)
 *    Bytecode : [ OP_SET_VAR | r0 | 0001 | 000000101010 ]
 * 
 * 3. Constantes Inmutables (const MAX = 100):
 *    Assembly : LOADK r2, #100 {CONSTRAINED|READONLY}
 *    C Macro  : FOXMODE_CREATE_iABx(OP_LOADK, 2, FOX_INST_FLAG_CONSTRAINED | FOX_INST_FLAG_READONLY, 100)
 *    Bytecode : [ OP_LOADK | r2 | 0011 | 000001100100 ]
 * 
 * 4. Control de Flujo con Salto Relativo:
 *    Assembly : JMP_IF_FALSE r0, offset(+12)
 *    C Macro  : FOXMODE_CREATE_iAsBx(OP_JMP_FALSE, 0, FOX_INST_FLAG_NONE, +12)
 *    Bytecode : [ OP_JMP_FALSE | r0 | 0000 | 100000001011 ] (sBx = 12 + 2047 = 2059)
 * ============================================================================
 */

typedef uint32_t FoxyInstruction;

#if FOXY_COMPILER_SUPPORTS_XMACROS

    /* ============================================================================
     * X-MACRO LIST: FOXY_INSTRUCTION_FLAG_LIST
     * ============================================================================
     * Formato: F(enum_symbol, bitmask_value)
     */
    #define FOXY_INSTRUCTION_FLAG_LIST(F) \
        F(FOX_INST_FLAG_NONE,        0x0u)      /* Sin restricciones (dinámico puro) */ \
        F(FOX_INST_FLAG_CONSTRAINED, (1u << 0)) /* Bit 0: Valida constraint_type en runtime */ \
        F(FOX_INST_FLAG_READONLY,    (1u << 1)) /* Bit 1: Protege contra reasignación (Inmutable) */ \
        F(FOX_INST_FLAG_AUTO_CAST,   (1u << 2)) /* Bit 2: Aplica coerción implícita si aplica */ \
        F(FOX_INST_FLAG_UNGUARDED,   (1u << 3)) /* Bit 3: Omite verificaciones en caliente (JIT/Fast-path) */

    /* Generación del enum a partir de la X-Macro */
    typedef enum {
        #define F(flag_enum, flag_val) flag_enum = flag_val,
        FOXY_INSTRUCTION_FLAG_LIST(F)
        #undef F
    } FoxyInstructionFlags;

#else // Fallback manual para compiladores heredados/limitados

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
#define FOXMODE_SHIFT_OPCODE           24u  /* Opcode ocupa bits [31:24] */
#define FOXMODE_SHIFT_A                16u  /* Registro Destino A ocupa bits [23:16] */
#define FOXMODE_SHIFT_FLAGS            12u  /* Flags de ejecución ocupan bits [15:12] */
#define FOXMODE_SHIFT_B                8u   /* Registro B (Layout iABC) ocupa bits [11:8] */

/* Máscaras de aislamiento de bits (Bitwise Masks) */
#define FOXMODE_MASK_8BIT              0xFFu   /* Mantiene valores de 8 bits (0 - 255) */
#define FOXMODE_MASK_4BIT              0x0Fu   /* Mantiene valores de 4 bits (0 - 15) */
#define FOXMODE_MASK_12BIT             0x0FFFu /* Mantiene valores de 12 bits (0 - 4095) */

/* Sesgo para codificación de saltos relativos con signo (Signed Offset) */
#define FOXMODE_SBX_BIAS               2047    /* Mapea rango [-2047, +2047] a [0, 4094] */

/* --- Extracción de Campos en Tiempo de Despacho (Dispatch Helpers) --- */
#define FOXMODE_GET_OPCODE(i)   ((uint8_t)(((i) >> FOXMODE_SHIFT_OPCODE) & FOXMODE_MASK_8BIT))
#define FOXMODE_GET_A(i)        ((uint8_t)(((i) >> FOXMODE_SHIFT_A) & FOXMODE_MASK_8BIT))
#define FOXMODE_GET_FLAGS(i)    ((uint8_t)(((i) >> FOXMODE_SHIFT_FLAGS) & FOXMODE_MASK_4BIT))

#define FOXMODE_GET_B_4B(i)     ((uint8_t)(((i) >> FOXMODE_SHIFT_B) & FOXMODE_MASK_4BIT))
#define FOXMODE_GET_C(i)        ((uint8_t)((i) & FOXMODE_MASK_8BIT))

#define FOXMODE_GET_BX(i)       ((uint16_t)((i) & FOXMODE_MASK_12BIT))
#define FOXMODE_GET_SBX(i)      ((int16_t)((int32_t)((i) & FOXMODE_MASK_12BIT) - FOXMODE_SBX_BIAS))

/* --- Emisión / Empaquetado (Codegen Helpers) --- */
#define FOXMODE_CREATE_iABC(op, a, b, flags, c) \
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

#define FOXMODE_CREATE_iAsBx(op, a, flags, sbx) \
    (((uint32_t)(op)    << FOXMODE_SHIFT_OPCODE) | \
     ((uint32_t)(a)     << FOXMODE_SHIFT_A) | \
     (((uint32_t)(flags) & FOXMODE_MASK_4BIT) << FOXMODE_SHIFT_FLAGS) | \
     ((uint32_t)((int32_t)(sbx) + FOXMODE_SBX_BIAS) & FOXMODE_MASK_12BIT))

#endif // F_FOXMODE_H