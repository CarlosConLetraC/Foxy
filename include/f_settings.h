#ifndef F_SETTINGS_H
    #define F_SETTINGS_H

    // Habilitar características extendidas de POSIX y GNU (necesario para strdup)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #ifndef _DEFAULT_SOURCE
        #define _DEFAULT_SOURCE
    #endif

    // Límites de la Máquina Virtual y Procesos
    #define FOXY_MAX_FRAMES      256
    #define FOXY_STACK_MAX       1024
    #define FOXY_MAX_LOCALS      256

    // Configuración por defecto de rutas o entorno
    #ifndef FOXY_DEFAULT_HOME
        #define FOXY_DEFAULT_HOME    "/opt/foxy-lang"
    #endif

    #ifndef FOXY_DEFAULT_CPATH
        #define FOXY_DEFAULT_CPATH "./?.so;./sys/?.so;./f_include/?.so;./?/init.so"
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