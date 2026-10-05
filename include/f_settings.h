#pragma once

// #include <stdint.h>
// #include <stdbool.h>
// #include <stddef.h>

/**
 * ============================================================================
 * FOXY-LANG SYSTEM CONFIGURATION & GLOBAL SETTINGS (f_settings.h)
 * ============================================================================
 * 
 * Este archivo define la configuración global, detección del entorno del
 * compilador, límites persistentes de memoria de la Máquina Virtual (VM),
 * constantes del sistema operativo y macros de exportación FFI/Shared Libs.
 * ============================================================================
 */

// /* --- Extensión de Características del Sistema (POSIX / GNU) --- */
// #ifndef _GNU_SOURCE
//     #define _GNU_SOURCE
// #endif
// #ifndef _DEFAULT_SOURCE
//     #define _DEFAULT_SOURCE
// #endif

/**
 * ----------------------------------------------------------------------------
 * DETECCIÓN DE OPTIMIZACIONES DEL COMPILADOR
 * ----------------------------------------------------------------------------
 * Detecta si el compilador soporta Computed GOTOs (punteros a etiquetas de GCC/Clang)
 * para habilitar el motor de despacho por "Direct Threaded Code" en el loop de la VM.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define USE_COMPUTED_GOTO 1
    #define FOXY_PACKED __attribute__((__packed__))
#else
    #define USE_COMPUTED_GOTO 0
    #define FOXY_PACKED
#endif

/**
 * ----------------------------------------------------------------------------
 * LÍMITES ARQUITECTÓNICOS Y CAPACIDADES PERSISTENTES DE LA VM
 * ----------------------------------------------------------------------------
 */

/** Longitud máxima de  */
#define LEXER_LINE_BUFFER_SIZE             (1u << 12)

/** Longitud máxima de identificadores de variables, funciones y símbolos (256 B). */
#define FOXY_MAX_IDENTIFIER_LEN            (1u << 8)

/** Tamaño máximo del nombre/ruta de módulo importado mapeado a FILENAME_MAX. */
#ifdef FILENAME_MAX
    #define FOXY_MAX_MODULE_NAME_SIZE      ((unsigned int)FILENAME_MAX)
#else
    #define FOXY_MAX_MODULE_NAME_SIZE      (1u << 9)   /* Fallback: 512 bytes */
#endif

/** Tamaño del buffer intermedio de nombres/cadenas basado en el BUFSIZ nativo de C. */
#ifdef BUFSIZ
    #define FOXY_NAME_BUFFER_SIZE          ((unsigned int)BUFSIZ)
#else
    #define FOXY_NAME_BUFFER_SIZE          (1u << 13)  /* Fallback: 8192 bytes */
#endif

/** Capacidad inicial de la tabla Hash interna de símbolos/atributos. */
#define FOXY_MAX_HASHTABLE_CAPACITY        (1u << 5)   /* 32 slots */

/** Capacidad inicial de nodos en la construcción del AST (Program Node). */
#define FOXY_INITIAL_PROGRAM_NODE_CAPACITY (1u << 4)   /* 16 nodos */

/** Profundidad máxima del Call Stack (CallFrames de funciones simultáneas). */
#define FOXY_MAX_FRAMES                    (1u << 8)   /* 256 frames */

/** Máximo de registros direccionables por marco (alineado al campo A de 8 bits). */
#define FOXY_MAX_REGISTERS_PER_FRAME       (1u << 8)   /* 256 registros (r0 <-> r255) */

/** Capacidad de la pila principal de evaluación de operandos de la VM. */
#define FOXY_MAX_STACK_CAPACITY            (1u << 8)   /* 256 FoxyValues */

/** Máximo de variables locales activas simultáneas en el léxico de un scope. */
#define FOXY_MAX_LOCALS                    (1u << 8)   /* 256 variables locales */

/** Máximo de Upvalues (variables libres capturadas) por Closure. */
#define FOXY_MAX_UPVALUES                  (1u << 8)   /* 256 upvalues */

/** Capacidad máxima de la pool de constantes de un chunk de bytecode (12 bits Bx). */
#define FOXY_MAX_CONSTANTS_CAPACITY        (1u << 18)  /* 262,144 constantes */

/** Capacidad de elementos de un arreglo In-line de foxy-lang (67,108,864) */
#define FOXY_ARRAY_MAX_STACK_CAPACITY      (1u << 26)

/**
 * ----------------------------------------------------------------------------
 * COMPATIBILIDAD DE COMPILADOR PARA X-MACROS
 * ----------------------------------------------------------------------------
 * Verifica el soporte de macros variádicas C99 / ISO C para el despliegue de
 * enumerados y dispatchers basados en el patrón X-Macro.
 */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
    (defined(__GNUC__) && __GNUC__ >= 3) || \
    (defined(_MSC_VER) && _MSC_VER >= 1400)
    #define FOXY_COMPILER_SUPPORTS_XMACROS 1
#else
    #define FOXY_COMPILER_SUPPORTS_XMACROS 0
#endif

/** Separador de rutas para las variables de entorno de módulos. */
#define FOXY_PATH_SEP ";"

/**
 * ----------------------------------------------------------------------------
 * INFORMACIÓN DE VERSIÓN DEL LENGUAJE (SEMVER)
 * ----------------------------------------------------------------------------
 */
#define FOXY_VERSION_MAJOR         1
#define FOXY_VERSION_MINOR         0
#define FOXY_VERSION_PATCH         0
#define FOXY_VERSION_STRING        "1.0.0"
#define FOXY_VERSION_NAME          "Foxy-Lang 1.0.0"

/**
 * ----------------------------------------------------------------------------
 * ADAPTACIÓN AL SISTEMA OPERATIVO Y RUTAS POR DEFECTO VERSIONADAS
 * ----------------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__MSDOS__) || defined(__DOS__)
    #define FOXY_DIR_DELIM          "\\"
    #define FOXY_MOD_EXT            ".dll"
    #define FOXY_DEFAULT_HOME       "C:\\foxy-lang\\" FOXY_VERSION_DIR
#else
    #define FOXY_DIR_DELIM          "/"
    #define FOXY_MOD_EXT            ".so"
    #define FOXY_DEFAULT_HOME       "/opt/foxy-lang/" FOXY_VERSION_DIR
#endif

/** Configuración de la ruta CPATH para búsqueda y carga de módulos C/FFI. */
#ifndef FOXY_DEFAULT_CPATH
    #define FOXY_DEFAULT_CPATH \
        "." FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
        "." FOXY_DIR_DELIM "?" FOXY_DIR_DELIM "init" FOXY_MOD_EXT FOXY_PATH_SEP \
        "." FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
        "." FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_DIR_DELIM "init" FOXY_MOD_EXT FOXY_PATH_SEP \
        FOXY_DEFAULT_HOME FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
        FOXY_DEFAULT_HOME FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_DIR_DELIM "init" FOXY_MOD_EXT
#endif

/**
 * ----------------------------------------------------------------------------
 * EXPORTACIÓN DE SÍMBOLOS (SHARED LIBRARIES / DYNAMIC LINKING)
 * ----------------------------------------------------------------------------
 */
#ifndef FOXY_EXPORT
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef FOXY_BUILDING_SHARED
            #define FOXY_EXPORT __declspec(dllexport)
        #else
            #define FOXY_EXPORT __declspec(dllimport)
        #endif
    #else
        #if __GNUC__ >= 4
            #define FOXY_EXPORT __attribute__((visibility("default")))
        #else
            #define FOXY_EXPORT
        #endif
    #endif
#endif

/**
 * ----------------------------------------------------------------------------
 * CONFIGURACIÓN Y CAPACIDADES DEL AST (f_ast)
 * ----------------------------------------------------------------------------
 */

/** Capacidad inicial por defecto para arreglos dinámicos del AST (nodos hijo, argumentos, etc.) */
#ifndef FOXY_AST_CHILDREN_INITIAL_CAPACITY
    #define FOXY_AST_CHILDREN_INITIAL_CAPACITY (1u << 3) /* 8 elementos */
#endif

/**
 * Tamaño máximo del buffer intermedio para el parseo de literales numéricos.
 * Alineado con límites de representación de números reales en C (IEEE 754 / quad precision).
 */
#ifndef FOXY_NUMERIC_LITERAL_BUFFER_SIZE
    #define FOXY_NUMERIC_LITERAL_BUFFER_SIZE (1u << 7) /* 128 bytes */
#endif
