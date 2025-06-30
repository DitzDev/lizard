#include "import.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"
#include <sys/stat.h>
#include <unistd.h>

ImportManager *import_manager_create(void) {
    ImportManager *manager = malloc(sizeof(ImportManager));
    manager->modules = NULL;
    return manager;
}

void import_manager_destroy(ImportManager *manager) {
    if (!manager) return;
    
    ImportedModule *current = manager->modules;
    while (current) {
        ImportedModule *next = current->next;
        free(current->name);
        free(current->path);
        environment_destroy(current->env);
        free(current);
        current = next;
    }
    
    free(manager);
}

static char *resolve_module_path(const char *module_path) {
    char *resolved_path = malloc(512);
    
    if (module_path[0] == '.' || module_path[0] == '/') {
        strcpy(resolved_path, module_path);
    } else {
        snprintf(resolved_path, 512, "./%s", module_path);
    }
    
    if (!strstr(resolved_path, ".lz")) {
        strcat(resolved_path, ".lz");
    }
    
    struct stat st;
    if (stat(resolved_path, &st) == 0) {
        return resolved_path;
    }
    
    free(resolved_path);
    return NULL;
}

static char *read_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';
    
    fclose(file);
    return content;
}

bool import_process_module(ImportManager *manager, Interpreter *interpreter, 
                          const char *module_path, const char *alias) {
    ImportedModule *current = manager->modules;
    while (current) {
        if (strcmp(current->path, module_path) == 0) {
            return true; // Already imported
        }
        current = current->next;
    }
    
    char *resolved_path = resolve_module_path(module_path);
    if (!resolved_path) {
        Position pos = {0, 0, (char*)module_path};
        error_report(ERROR_IMPORT, pos, "Module not found", 
                   "Check if the module file exists and the path is correct");
        return false;
    }

    char *source = read_file(resolved_path);
    if (!source) {
        Position pos = {0, 0, (char*)module_path};
        error_report(ERROR_IMPORT, pos, "Cannot read module file", 
                   "Check file permissions and accessibility");
        free(resolved_path);
        return false;
    }
    
    Lexer *lexer = lexer_create(source, resolved_path);
    Token *tokens = lexer_tokenize(lexer);
    
    Parser *parser = parser_create(tokens, lexer->token_count);
    ASTNode *ast = parser_parse(parser);
    
    if (!ast) {
        Position pos = {0, 0, resolved_path};
        error_report(ERROR_IMPORT, pos, "Failed to parse module", 
                   "Check module syntax");
        free(source);
        free(resolved_path);
        lexer_destroy(lexer);
        parser_destroy(parser);
        return false;
    }
    
    Environment *module_env = environment_create(NULL);
    
    Interpreter *module_interpreter = interpreter_create();
    module_interpreter->global_env = module_env;
    module_interpreter->current_env = module_env;
    
    interpreter_run(module_interpreter, ast);
    
    ImportedModule *new_module = malloc(sizeof(ImportedModule));
    new_module->name = strdup(alias ? alias : module_path);
    new_module->path = strdup(resolved_path);
    new_module->env = module_env;
    new_module->next = manager->modules;
    manager->modules = new_module;
    
    EnvEntry *entry = module_env->entries;
    while (entry) {
        if (entry->value->type == VALUE_FUNCTION) {
            Function *func = entry->value->function_val;
            if (func->is_public) {
                char qualified_name[256];
                snprintf(qualified_name, sizeof(qualified_name), "%s.%s", 
                        new_module->name, entry->name);
                environment_define_default(interpreter->current_env, qualified_name, 
                                 entry->value, entry->type);
            }
        }
        entry = entry->next;
    }
    
    free(source);
    free(resolved_path);
    ast_destroy(ast);
    lexer_destroy(lexer);
    parser_destroy(parser);
    
    module_interpreter->global_env = NULL;
    interpreter_destroy(module_interpreter);
    
    return true;
}

bool import_process_statement(ImportManager *manager, Interpreter *interpreter, 
                             ASTNode *import_node) {
    if (!import_node || import_node->type != AST_IMPORT_STATEMENT) {
        return false;
    }
    
    const char *module_path = import_node->import_statement.module_path;
    
    if (import_node->import_statement.name_count == 0) {
        return import_process_module(manager, interpreter, module_path, NULL);
    } else {
        // import { func1, func2 } from "module" as alias
        char *alias = NULL;
        if (import_node->import_statement.aliases && 
            import_node->import_statement.aliases[0]) {
            alias = import_node->import_statement.aliases[0];
        }
        
        bool success = import_process_module(manager, interpreter, module_path, alias);
        if (!success) return false;
        
        Environment *module_env = import_get_module_environment(manager, 
                                    alias ? alias : module_path);
        if (!module_env) return false;
        
        for (int i = 0; i < import_node->import_statement.name_count; i++) {
            const char *func_name = import_node->import_statement.names[i];
            Value *func_value = environment_get(module_env, func_name);
            
            if (func_value && func_value->type == VALUE_FUNCTION) {
                Function *func = func_value->function_val;
                if (func->is_public) {
                    environment_define_default(interpreter->current_env, func_name, 
                                     func_value, "function");
                } else {
                    Position pos = import_node->pos;
                    error_report(ERROR_IMPORT, pos, "Function is not public", 
                               "Only public functions can be imported");
                    return false;
                }
            } else {
                Position pos = import_node->pos;
                error_report(ERROR_IMPORT, pos, "Function not found in module", 
                           "Check if the function exists and is spelled correctly");
                return false;
            }
        }
    }
    
    return true;
}

Environment *import_get_module_environment(ImportManager *manager, const char *name) {
    ImportedModule *current = manager->modules;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->env;
        }
        current = current->next;
    }
    return NULL;
}