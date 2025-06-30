#include "environment.h"
#include <stdlib.h>
#include <string.h>

Environment *environment_create(Environment *parent) {
    Environment *env = malloc(sizeof(Environment));
    if (!env) return NULL;
    
    env->entries = NULL;
    env->parent = parent;
    return env;
}

void environment_destroy(Environment *env) {
    if (!env) return;
    
    EnvEntry *current = env->entries;
    while (current) {
        EnvEntry *next = current->next;
        free(current->name);
        free(current->type);
        value_destroy(current->value);
        free(current);
        current = next;
    }
    
    free(env);
}

bool environment_define(Environment *env, const char *name, Value *value, const char *type, bool is_fixed) {
    if (!env || !name) return false;
    
    EnvEntry *current = env->entries;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return false;
        }
        current = current->next;
    }
   
    EnvEntry *new_entry = malloc(sizeof(EnvEntry));
    if (!new_entry) return false;
    
    new_entry->name = strdup(name);
    new_entry->value = value ? value_copy(value) : NULL;
    new_entry->type = type ? strdup(type) : NULL;
    new_entry->is_fixed = is_fixed;
    new_entry->is_initialized = (value != NULL);
    new_entry->next = env->entries; 
    
    env->entries = new_entry;
    return true;
}

Value *environment_get(Environment *env, const char *name) {
    if (!env || !name) return NULL;
    
    Environment *current_env = env;
    while (current_env) {
        EnvEntry *current = current_env->entries;
        while (current) {
            if (strcmp(current->name, name) == 0) {
                return current->value;
            }
            current = current->next;
        }
        current_env = current_env->parent;
    }
    
    return NULL;
}

bool environment_exists(Environment *env, const char *name) {
    return environment_get(env, name) != NULL;
}

bool environment_set(Environment *env, const char *name, Value *value) {
    if (!env || !name || !value) return false;
    
    Environment *current_env = env;
    while (current_env) {
        EnvEntry *current = current_env->entries;
        while (current) {
            if (strcmp(current->name, name) == 0) {
                if (current->is_fixed && current->is_initialized) {
                    return false;
                }
                
                value_destroy(current->value);
                current->value = value_copy(value);
                current->is_initialized = true;
                return true;
            }
            current = current->next;
        }
        current_env = current_env->parent;
    }
    
    return false;
}

EnvEntry *environment_get_entry(Environment *env, const char *name) {
    if (!env || !name) return NULL;
    
    Environment *current_env = env;
    while (current_env) {
        EnvEntry *current = current_env->entries;
        while (current) {
            if (strcmp(current->name, name) == 0) {
                return current;
            }
            current = current->next;
        }
        current_env = current_env->parent;
    }
    
    return NULL;
}