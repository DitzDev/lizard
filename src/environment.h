#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "value.h"
#include <stdbool.h>

typedef struct EnvEntry {
    char *name;
    Value *value;
    char *type;
    struct EnvEntry *next;
} EnvEntry;

typedef struct Environment {
    EnvEntry *entries;
    struct Environment *parent;
} Environment;

Environment *environment_create(Environment *parent);
void environment_destroy(Environment *env);
bool environment_define(Environment *env, const char *name, Value *value, const char *type);
Value *environment_get(Environment *env, const char *name);
bool environment_exists(Environment *env, const char *name);
bool environment_set(Environment *env, const char *name, Value *value);

#endif