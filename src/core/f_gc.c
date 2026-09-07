#include "f_settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "uthash.h"
#include "f_gc.h"
#include "f_dict.h"
#include "f_value.h"
#include "f_function.h"
#include "f_object.h"
#include "f_class.h"
#include "f_env.h"

typedef struct InternalGNode {
    FoxyHeapType type;
    bool is_marked;
    struct InternalGNode *next;
    void (*free_func)(void*);
    char data[];
} InternalGNode;

static struct {
    InternalGNode *head;
    size_t total_allocated_objects;
} GC_State = {NULL, 0};

void f_gc_init(void) {
    GC_State.head = NULL;
    GC_State.total_allocated_objects = 0;
}

void* f_gc_allocate(FoxyHeapType type, size_t size, void (*free_func)(void*)) {
    InternalGNode *node = (InternalGNode*)malloc(sizeof(InternalGNode) + size);
    if (!node) {
        fprintf(stderr, "[Foxy GC Error] Memoria agotada en el heap\n");
        return NULL;
    }

    node->type = type;
    node->is_marked = false;
    node->free_func = free_func;

    node->next = GC_State.head;
    GC_State.head = node;

    GC_State.total_allocated_objects++;
    return (void*)node->data;
}

void f_gc_mark_value(FoxyValue value) {
    void *target_ptr = NULL;

    switch (value.type) {
        case FOXY_VAL_DICT:
            target_ptr = value.as.dict;
            break;
        case FOXY_VAL_OBJECT:
            target_ptr = value.as.obj;
            break;
        case FOXY_VAL_FUNCTION:
            target_ptr = value.as.func;
            break;
        case FOXY_VAL_CLASS:
            target_ptr = value.as.klass;
            break;
        default:
            return; 
    }

    if (target_ptr) {
        InternalGNode *node = (InternalGNode*)((char*)target_ptr - offsetof(InternalGNode, data));
        
        if (node && !node->is_marked) {
            node->is_marked = true;

            switch (node->type) {
                case FOXY_HEAP_DICT: {
                    FoxyDict *dict = (FoxyDict*)node->data;
                    FoxyDictEntry *current, *tmp;
                    HASH_ITER(hh, dict->head, current, tmp) {
                        f_gc_mark_value(current->value);
                    }
                    break;
                }
                case FOXY_HEAP_OBJECT: {
                    FoxyObject *obj = (FoxyObject*)node->data;
                    if (obj->klass) f_gc_mark_root(obj->klass);
                    for (size_t i = 0; i < obj->field_count; i++) f_gc_mark_value(obj->fields[i].value);
                    break;
                }
                case FOXY_HEAP_CLASS: {
                    FoxyClass *klass = (FoxyClass*)node->data;
                    if (klass->super_class) f_gc_mark_root(klass->super_class);
                    break;
                }
                case FOXY_HEAP_ENV: {
                    FoxyEnv *env = (FoxyEnv*)node->data;
                    if (env->parent) f_gc_mark_root(env->parent);
                    if (env->bindings) {
                        f_gc_mark_root(env->bindings);
                        FoxyDictEntry *current, *tmp;
                        HASH_ITER(hh, env->bindings->head, current, tmp) {
                            f_gc_mark_value(current->value);
                        }
                    }
                    break;
                }
                case FOXY_HEAP_FUNCTION: {
                    FoxyFunction *func = (FoxyFunction*)node->data;
                    if (func->type == FOXY_FUNCTION_USER) {
                        if (func->as.user.env) f_gc_mark_root(func->as.user.env);
                        if (func->as.user.constants) {
                            for (size_t i = 0; i < func->as.user.constants_count; i++) f_gc_mark_value(func->as.user.constants[i]);
                        }
                    }
                    break;
                }
            }
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

    while (current != NULL) {
        if (!current->is_marked) {
            InternalGNode *to_delete = current;

            if (prev == NULL)
                GC_State.head = current->next;
            else
                prev->next = current->next;

            current = current->next;

            if (to_delete->free_func)
                to_delete->free_func((void*)to_delete->data);

            free(to_delete);
            
            if (GC_State.total_allocated_objects > 0)
                GC_State.total_allocated_objects--;
        } else {
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