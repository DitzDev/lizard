#include "environment.h"
#include <string.h>
#include <stdlib.h>

Environment *environment_create(Environment *parent) {
    Environment *env = malloc(sizeof(Environment));
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

void environment_define(Environment *env, const char *name, Value *value, const char *type) {
    EnvEntry *entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->value = value_copy(value);
    entry->type = type ? strdup(type) : NULL;
    entry->next = env->entries;
    env->entries = entry;
}

Value *environment_get(Environment *env, const char *name) {
    EnvEntry *current = env->entries;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    if (env->parent) {
        return environment_get(env->parent, name);
    }
    
    return NULL;
}

bool environment_exists(Environment *env, const char *name) {
    return environment_get(env, name) != NULL;
}

void environment_set(Environment *env, const char *name, Value *value) {
    EnvEntry *current = env->entries;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            value_destroy(current->value);
            current->value = value_copy(value);
            return;
        }
        current = current->next;
    }
    
    if (env->parent) {
        environment_set(env->parent, name, value);
    }
}