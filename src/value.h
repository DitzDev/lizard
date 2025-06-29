#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"

typedef enum {
    VALUE_NULL,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_FUNCTION
} ValueType;

typedef struct Value Value;
typedef struct Function Function;

struct Function {
    char *name;
    char **param_names;
    char **param_types;
    struct ASTNode **param_defaults; // New: default values
    bool *param_has_default;         // New: track which params have defaults
    int param_count;
    char *return_type;
    struct ASTNode *body;
    bool is_public;
    Position declaration_pos;
};

struct Value {
    ValueType type;
    union {
        int int_val;
        double float_val;
        char *string_val;
        bool bool_val;
        Function *function_val;
    };
};

Value *value_create_int(int val);
Value *value_create_float(double val);
Value *value_create_string(const char *val);
Value *value_create_bool(bool val);
Value *value_create_function(Function *func);
Value *value_create_null(void);
void value_destroy(Value *value);
Value *value_copy(Value *value);
void value_print(Value *value);
char *value_to_string(Value *value);
const char *value_type_to_string(ValueType type);

Function *function_create(const char *name, char **param_names, char **param_types, 
                         struct ASTNode **param_defaults, bool *param_has_default,
                         int param_count, const char *return_type, struct ASTNode *body, 
                         bool is_public, Position declaration_pos);
void function_destroy(Function *func);

char *infer_type_from_value(Value *value);

#endif