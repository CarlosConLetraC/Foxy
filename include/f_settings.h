#ifndef F_SETTINGS_H
    #define F_SETTINGS_H
    typedef unsigned int uint;

    // Habilitar características extendidas de POSIX y GNU (necesario para strdup)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #ifndef _DEFAULT_SOURCE
        #define _DEFAULT_SOURCE
    #endif

    // Límites de la Máquina Virtual y Procesos
    #define FOXY_MAX_STACK_SIZE            (1u << 3)  // 8
    #define FOXY_MAX_CALL_STACK_SIZE       (1u << 3)  // 8
    #define FOXY_MAX_LOCALS_CAPACITY       (1u << 4)  // 16
    #define FOXY_MAX_PROGRAM_NODE_CAPACITY (1u << 4)  // 16
    #define FOXY_MAX_TABLE_CAPACITY        (1u << 5)  // 32
    #define FOXY_MAX_CONSTANTS_CAPACITY    (1u << 6)  // 64
    #define FOXY_MAX_IDENTIFIER_LEN        (1u << 7)  // 128
    #define FOXY_MAX_FRAMES                (1u << 8)  // 256
    #define FOXY_MAX_CODE_CAPACITY         (1u << 8)  // 256
    #define FOXY_MAX_STACK_CAPACITY        (1u << 8)  // 256
    #define FOXY_MAX_LOCALS                (1u << 8)  // 256
    #define FOXY_MAX_MODULE_NAME_SIZE      (1u << 9)  // 512
    #define FOXY_NAME_BUFFER_SIZE          (1u << 10) // 1024

    #define FOXY_NULL_VALUE ((FoxyValue){ .type = FOXY_VAL_NULL, .as.ptr = NULL })

    // Configuración por defecto de rutas o entorno
    #ifndef FOXY_DEFAULT_HOME
        #define FOXY_DEFAULT_HOME    "/opt/foxy-lang"
    #endif

    #ifndef FOXY_DEFAULT_CPATH
        #define FOXY_DEFAULT_CPATH FOXY_DEFAULT_HOME "/f_include/?.so;./?.so;./sys/?.so;./f_include/?.so;./?/init.so"
    #endif

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