#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"
#include "environment.h"
#include "value.h"

typedef struct {
    Environment *global_env;
    Environment *current_env;
    bool return_flag;
    Value *return_value;
    Function *current_function;
} Interpreter;

Interpreter *interpreter_create(void);
void interpreter_destroy(Interpreter *interpreter);
Value *interpreter_evaluate(Interpreter *interpreter, ASTNode *node);
void interpreter_run(Interpreter *interpreter, ASTNode *ast);

#endif