#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "value.h"
#include <stdbool.h>

typedef struct EnvEntry {
    char *name;
    Value *value;
    char *type;
    bool is_fixed;
    bool is_initialized;
    struct EnvEntry *next;
} EnvEntry;

typedef struct Environment {
    EnvEntry *entries;
    struct Environment *parent;
} Environment;

Environment *environment_create(Environment *parent);
void environment_destroy(Environment *env);
bool environment_define(Environment *env, const char *name, Value *value, const char *type, bool is_fixed);
Value *environment_get(Environment *env, const char *name);
bool environment_exists(Environment *env, const char *name);
bool environment_set(Environment *env, const char *name, Value *value);
EnvEntry *environment_get_entry(Environment *env, const char *name);  

#define environment_define_default(env, name, value, type) \
    environment_define(env, name, value, type, false)

#endif