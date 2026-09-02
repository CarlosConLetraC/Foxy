#ifndef F_FUNCTION_H
    #define F_FUNCTION_H

    #include <stddef.h>
    #include <stdint.h>
    #include "f_constant.h" // Asegúrate de incluir la definición de FoxyConstant

    typedef struct FoxyFunction {
        char *name;                 // Nombre de la función (ej. "calcula_suma" o NULL si es anónima)
        uint8_t arity;              // Número de parámetros que acepta la función
        
        // Bytecode exclusivo de la función
        uint8_t *code;              // Arreglo dinámico de opcodes
        size_t code_size;           // Tamaño actual del bytecode
        size_t code_capacity;       // Capacidad asignada al búfer de bytecode

        // Constant Pool exclusivo de la función
        FoxyConstant *constants;    // Arreglo de constantes locales de la función
        size_t constants_count;     // Cantidad de constantes registradas
        size_t constants_capacity;  // Capacidad del búfer de constantes    
        uint8_t *bytecode;
        uint16_t locals_count;      // Cantidad de variables locales que requiere la función

    } FoxyFunction;

    // Constructor y destructor de la estructura
    FoxyFunction* f_function_create(const char *name, uint8_t arity);
    void f_function_free(FoxyFunction *func);
#endif // F_FUNCTION_H