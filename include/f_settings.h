#ifndef F_SETTINGS_H
#define F_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

/* --- Extensión de Características del Sistema (POSIX / GNU) --- */
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE
#endif

/**
 * ----------------------------------------------------------------------------
 * DETECCIÓN DE OPTIMIZACIONES DEL COMPILADOR
 * ----------------------------------------------------------------------------
 * Detecta si el compilador soporta Computed GOTOs (punteros a etiquetas de GCC/Clang)
 * para habilitar el motor de despacho por "Direct Threaded Code" en el loop de la VM.
 */
#ifndef USE_COMPUTED_GOTO
    #if defined(__GNUC__) || defined(__clang__)
        #define USE_COMPUTED_GOTO 1
    #else
        #define USE_COMPUTED_GOTO 0
    #endif
#endif

/**
 * ----------------------------------------------------------------------------
 * LÍMITES ARQUITECTÓNICOS Y CAPACIDADES PERSISTENTES DE LA VM
 * ----------------------------------------------------------------------------
 */

/** Tamaño del buffer estático para caracteres ASCII/Lexer. */
#define FOXY_MAX_ASCII_SIZE            (1u << 8)   /* 256 */

/** Longitud máxima de identificadores de variables, funciones y símbolos. */
#define FOXY_MAX_IDENTIFIER_LEN        (1u << 8)   /* 256 bytes */

/** Tamaño máximo permitido para el nombre de un módulo importado. */
#define FOXY_MAX_MODULE_NAME_SIZE      (1u << 9)   /* 512 bytes */

/** Tamaño del buffer intermedio de nombres para resolución de símbolos/paths. */
#define FOXY_MAX_NAME_BUFFER_SIZE      (1u << 10)  /* 1024 bytes */

/** Capacidad inicial de la tabla Hash interna de símbolos/atributos. */
#define FOXY_MAX_TABLE_CAPACITY        (1u << 5)   /* 32 slots */

/** Capacidad inicial de nodos en la construcción del AST (Program Node). */
#define FOXY_MAX_PROGRAM_NODE_CAPACITY (1u << 4)   /* 16 nodos */

/** Profundidad máxima del Call Stack (CallFrames de funciones simultáneas). */
#define FOXY_MAX_FRAMES                (1u << 8)   /* 256 frames */

/** Máximo de registros direccionables por marco (alineado al campo A de 8 bits). */
#define FOXY_MAX_REGISTERS_PER_FRAME   (1u << 8)   /* 256 registros (r0 - r255) */

/** Capacidad de la pila principal de evaluación de operandos de la VM. */
#define FOXY_MAX_STACK_CAPACITY        (1u << 8)   /* 256 FoxyValues */

/** Máximo de variables locales activas simultáneas en el léxico de un scope. */
#define FOXY_MAX_LOCALS                (1u << 8)   /* 256 variables locales */

/** Máximo de Upvalues (variables libres capturadas) por Closure. */
#define FOXY_MAX_UPVALUES              (1u << 8)   /* 256 upvalues */

/** Capacidad máxima de la pool de constantes de un chunk de bytecode (12 bits Bx). */
#define FOXY_MAX_CONSTANTS_CAPACITY    (1u << 18)  /* 262,144 constantes */

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
 * ADAPTACIÓN AL SISTEMA OPERATIVO Y RUTAS POR DEFECTO
 * ----------------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__MSDOS__) || defined(__DOS__)
    #define FOXY_DIR_DELIM          "\\"
    #define FOXY_MOD_EXT            ".dll"
    #ifndef FOXY_DEFAULT_HOME
        #define FOXY_DEFAULT_HOME   "C:\\foxy-lang"
    #endif
#else
    #define FOXY_DIR_DELIM          "/"
    #define FOXY_MOD_EXT            ".so"
    #ifndef FOXY_DEFAULT_HOME
        #define FOXY_DEFAULT_HOME   "/opt/foxy-lang"
    #endif
#endif

/** Configuración de la ruta CPATH para búsqueda y carga de módulos C/FFI. */
#ifndef FOXY_DEFAULT_CPATH
    #define FOXY_DEFAULT_CPATH FOXY_DEFAULT_HOME FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
                               "." FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
                               "." FOXY_DIR_DELIM "sys" FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
                               "." FOXY_DIR_DELIM "f_include" FOXY_DIR_DELIM "?" FOXY_MOD_EXT FOXY_PATH_SEP \
                               "." FOXY_DIR_DELIM "?" FOXY_DIR_DELIM "init" FOXY_MOD_EXT
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

#endif // F_SETTINGS_H