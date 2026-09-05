#include "f_settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "f_gc.h"
#include "f_dict.h"
#include "f_value.h"

// Estructura interna del nodo de control del GC en el heap
typedef struct InternalGNode {
    FoxyHeapType type;
    bool is_marked;
    struct InternalGNode *next;
    void (*free_func)(void*);
    char data[]; // Flexible array member para alojar el objeto real
} InternalGNode;

// Contexto global del Garbage Collector
static struct {
    InternalGNode *head;
    size_t total_allocated_objects;
} GC_State = {NULL, 0};

void f_gc_init(void) {
    GC_State.head = NULL;
    GC_State.total_allocated_objects = 0;
}

void* f_gc_allocate(FoxyHeapType type, size_t size, void (*free_func)(void*)) {
    // Reservamos espacio para la cabecera de control + el tamaño del objeto real
    InternalGNode *node = (InternalGNode*)malloc(sizeof(InternalGNode) + size);
    if (!node) {
        fprintf(stderr, "[Foxy GC Error] Memoria agotada al asignar objeto en el heap\n");
        return NULL;
    }

    node->type = type;
    node->is_marked = false;
    node->free_func = free_func;

    // Enlazar al inicio de la lista global del GC
    node->next = GC_State.head;
    GC_State.head = node;

    GC_State.total_allocated_objects++;

    // Retornamos el puntero al área de datos útiles (payload)
    return (void*)node->data;
}

void f_gc_mark_value(FoxyValue value) {
    void *target_ptr = NULL;

    // Extraemos el puntero del heap según el tipo definido en f_value.h
    switch (value.type) {
        case FOXY_VAL_DICT:
            target_ptr = value.as.dict;
            break;
        case FOXY_VAL_OBJECT:
            target_ptr = value.as.obj; // o value.as.object
            break;
        case FOXY_VAL_FUNCTION:
            target_ptr = value.as.func;
            break;
        case FOXY_VAL_CLASS:
            target_ptr = value.as.klass;
            break;
        default:
            return; // Tipos primitivos o sin gestión en heap no se marcan
    }

    if (target_ptr) {
        // Obtenemos la cabecera InternalGNode restando el offset del arreglo flexible
        InternalGNode *node = (InternalGNode*)((char*)target_ptr - offsetof(InternalGNode, data));
        
        if (node && !node->is_marked) {
            node->is_marked = true;

            // Marcado recursivo de dependencias internas según el tipo de objeto
            if (node->type == FOXY_HEAP_DICT) {
                FoxyDict *dict = (FoxyDict*)node->data;
                FoxyDictEntry *current, *tmp;
                HASH_ITER(hh, dict->head, current, tmp) {
                    f_gc_mark_value(current->value);
                }
            }
            // TODO: gregar la recursión para object, class y function conforme implementes sus estructuras internas.
        }
    }
}

void f_gc_mark_root(void *object_ptr) {
    if (!object_ptr) return;
    InternalGNode *node = (InternalGNode*)((char*)object_ptr - offsetof(InternalGNode, data));
    if (node) node->is_marked = true;
}

void f_gc_collect(void) {
    InternalGNode *current = GC_State.head;
    InternalGNode *prev = NULL;

    // Fase de Sweep (Barrer objetos no marcados)
    while (current != NULL) {
        if (!current->is_marked) {
            InternalGNode *to_delete = current;

            if (prev == NULL)
                GC_State.head = current->next;
            else
                prev->next = current->next;

            current = current->next;

            // Ejecutar el destructor específico si fue provisto
            if (to_delete->free_func)
                to_delete->free_func((void*)to_delete->data);

            free(to_delete);
            
            if (GC_State.total_allocated_objects > 0)
                GC_State.total_allocated_objects--;
        } else {
            // El objeto sobrevivió al ciclo, reseteamos la marca para el siguiente barrido
            current->is_marked = false;
            prev = current;
            current = current->next;
        }
    }
}

void f_gc_shutdown(void) {
    InternalGNode *current = GC_State.head;
    while (current != NULL) {
        InternalGNode *next = current->next;
        if (current->free_func)
            current->free_func((void*)current->data);
        free(current);
        current = next;
    }
    GC_State.head = NULL;
    GC_State.total_allocated_objects = 0;
}

size_t f_gc_get_allocated_count(void) {
    return GC_State.total_allocated_objects;
}