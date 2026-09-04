#include "f_symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FoxySymbolTable* f_symtable_new(void) {
    FoxySymbolTable *table = (FoxySymbolTable *)malloc(sizeof(FoxySymbolTable));
    if (!table) return NULL;

    table->capacity = 32;
    table->count = 0;
    table->next_id = 1; // Estilo AUTO_INCREMENT
    table->rows = (FoxySymbolRow *)malloc(sizeof(FoxySymbolRow) * table->capacity);
    
    if (!table->rows) {
        free(table);
        return NULL;
    }
    return table;
}

void f_symtable_free(FoxySymbolTable *table) {
    if (!table) return;
    if (table->rows) {
        free(table->rows);
    }
    free(table);
}

uint32_t f_symtable_insert(FoxySymbolTable *table, uint32_t id_module, const char *name, FoxyValue value) {
    if (!table || !name) return 0;

    // Verificar si el símbolo ya existe en el mismo módulo (Actualización / Upsert)
    for (size_t i = 0; i < table->count; i++) {
        if (table->rows[i].id_module == id_module && strcmp(table->rows[i].name, name) == 0) {
            table->rows[i].value = value;
            return table->rows[i].id_symbol;
        }
    }

    // Ampliar capacidad si es necesario (Reallocation estilo motor relacional)
    if (table->count >= table->capacity) {
        size_t new_cap = table->capacity * 2;
        FoxySymbolRow *new_rows = (FoxySymbolRow *)realloc(table->rows, sizeof(FoxySymbolRow) * new_cap);
        if (!new_rows) {
            fprintf(stderr, "[Foxy Error] Out of memory expanding FoxySymbolTable\n");
            return 0;
        }
        table->rows = new_rows;
        table->capacity = new_cap;
    }

    // Insertar nueva fila
    uint32_t assigned_id = table->next_id++;
    FoxySymbolRow *row = &table->rows[table->count++];
    row->id_symbol = assigned_id;
    row->id_module = id_module;
    
    strncpy(row->name, name, sizeof(row->name) - 1);
    row->name[sizeof(row->name) - 1] = '\0';
    
    row->value = value;

    return assigned_id;
}

FoxySymbolRow* f_symtable_find_by_name(FoxySymbolTable *table, const char *name) {
    if (!table || !name) return NULL;

    // Consulta indexada por nombre (Índice único)
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->rows[i].name, name) == 0) {
            return &table->rows[i];
        }
    }
    return NULL;
}

FoxySymbolRow* f_symtable_find_by_id(FoxySymbolTable *table, uint32_t id_symbol) {
    if (!table) return NULL;

    // Consulta por Clave Primaria (Primary Key Lookup)
    for (size_t i = 0; i < table->count; i++) {
        if (table->rows[i].id_symbol == id_symbol) {
            return &table->rows[i];
        }
    }
    return NULL;
}

bool f_symtable_delete_by_module(FoxySymbolTable *table, uint32_t id_module) {
    if (!table) return false;

    // Simulación de borrado en cascada (ON DELETE CASCADE)
    size_t i = 0;
    while (i < table->count) {
        if (table->rows[i].id_module == id_module) {
            // Desplazar elementos restantes (Compactar tabla)
            for (size_t j = i; j < table->count - 1; j++) {
                table->rows[j] = table->rows[j + 1];
            }
            table->count--;
        } else {
            i++;
        }
    }
    return true;
}