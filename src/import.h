#ifndef IMPORT_H
#define IMPORT_H

#include "parser.h"
#include "environment.h"
#include "interpreter.h"
#include <stdbool.h>

typedef struct ImportedModule {
    char *name;
    char *path;
    Environment *env;
    struct ImportedModule *next;
} ImportedModule;

typedef struct {
    ImportedModule *modules;
} ImportManager;

ImportManager *import_manager_create(void);
void import_manager_destroy(ImportManager *manager);
bool import_process_module(ImportManager *manager, Interpreter *interpreter, 
                          const char *module_path, const char *alias);
bool import_process_statement(ImportManager *manager, Interpreter *interpreter, 
                             ASTNode *import_node);
Environment *import_get_module_environment(ImportManager *manager, const char *name);

#endif