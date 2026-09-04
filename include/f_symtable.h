#ifndef F_SYMTABLE_H
    #define F_SYMTABLE_H

    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include "f_value.h"

    // Representación de una entidad de símbolo relacional (Fila)
    typedef struct {
        uint32_t id_symbol;       // Llave Primaria (PK)
        uint32_t id_module;       // Llave Foránea (FK) hacia la tabla de módulos
        char name[128];           // Nombre único del símbolo (ej: "printf", "out")
        FoxyValue value;          // El valor asociado (función, objeto, etc.)
    } FoxySymbolRow;

    // Tabla relacional de símbolos
    typedef struct {
        FoxySymbolRow *rows;
        size_t count;
        size_t capacity;
        uint32_t next_id;         // Auto-incremento estilo AUTO_INCREMENT
    } FoxySymbolTable;

    // Prototipos del subsistema relacional
    FoxySymbolTable* f_symtable_new(void);
    void f_symtable_free(FoxySymbolTable *table);

    // Operaciones estilo SQL / CRUD optimizadas en memoria
    uint32_t f_symtable_insert(FoxySymbolTable *table, uint32_t id_module, const char *name, FoxyValue value);
    FoxySymbolRow* f_symtable_find_by_name(FoxySymbolTable *table, const char *name);
    FoxySymbolRow* f_symtable_find_by_id(FoxySymbolTable *table, uint32_t id_symbol);
    bool f_symtable_delete_by_module(FoxySymbolTable *table, uint32_t id_module); // Cascada ON DELETE CASCADE
#endif // F_SYMTABLE_H