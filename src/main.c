#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "import.h"
#include "error.h"

#define VERSION "1.0.0"

void print_usage(const char *program_name) {
    printf("Lizard Programming Language Interpreter v%s\n", VERSION);
    printf("Usage: %s [options] [file]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -i, --interactive  Start interactive mode (REPL)\n");
    printf("\nExamples:\n");
    printf("  %s hello.lz      # Run hello.lz file\n", program_name);
    printf("  %s -i            # Start interactive mode\n", program_name);
}

void print_version(void) {
    printf("Lizard Programming Language v%s\n", VERSION);
    printf("Built with love for learning and experimentation.\n");
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(length + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    size_t read_length = fread(content, 1, length, file);
    content[read_length] = '\0';
    
    fclose(file);
    return content;
}

bool execute_file(const char *filename) {
    char *source = read_file(filename);
    if (!source) {
        return false;
    }
    
    // Create lexer
    Lexer *lexer = lexer_create(source, filename);
    if (!lexer) {
        fprintf(stderr, "Error: Failed to create lexer\n");
        free(source);
        return false;
    }
    
    // Tokenize
    Token *tokens = lexer_tokenize(lexer);
    if (!tokens) {
        fprintf(stderr, "Error: Tokenization failed\n");
        lexer_destroy(lexer);
        free(source);
        return false;
    }
    
    // Create parser
    Parser *parser = parser_create(tokens, lexer->token_count);
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser\n");
        lexer_destroy(lexer);
        free(source);
        return false;
    }
    
    // Parse
    ASTNode *ast = parser_parse(parser);
    if (!ast) {
        fprintf(stderr, "Error: Parsing failed\n");
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return false;
    }
    
    // Process imports first
    ImportManager *import_manager = import_manager_create();
    Interpreter *interpreter = interpreter_create();
    
    // Find and process import statements
    if (ast->type == AST_PROGRAM) {
        for (int i = 0; i < ast->program.statement_count; i++) {
            if (ast->program.statements[i]->type == AST_IMPORT_STATEMENT) {
                if (!import_process_statement(import_manager, interpreter, 
                                            ast->program.statements[i])) {
                    fprintf(stderr, "Error: Import processing failed\n");
                    import_manager_destroy(import_manager);
                    interpreter_destroy(interpreter);
                    ast_destroy(ast);
                    parser_destroy(parser);
                    lexer_destroy(lexer);
                    free(source);
                    return false;
                }
            }
        }
    }
    
    // Execute the program
    interpreter_run(interpreter, ast);
    
    // Cleanup
    import_manager_destroy(import_manager);
    interpreter_destroy(interpreter);
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(source);
    
    return true;
}

void interactive_mode(void) {
    printf("Lizard Programming Language v%s - Interactive Mode\n", VERSION);
    printf("Type 'exit' or press Ctrl+C to quit.\n\n");
    
    ImportManager *import_manager = import_manager_create();
    Interpreter *interpreter = interpreter_create();
    
    char input[1024];
    int line_number = 1;
    
    while (1) {
        printf("lizard:%d> ", line_number);
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        
        // Check for exit command
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        
        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }
        
        // Create temporary filename for REPL
        char temp_filename[64];
        snprintf(temp_filename, sizeof(temp_filename), "<repl:%d>", line_number);
        
        // Create lexer
        Lexer *lexer = lexer_create(input, temp_filename);
        if (!lexer) {
            printf("Error: Failed to create lexer\n");
            continue;
        }
        
        // Tokenize
        Token *tokens = lexer_tokenize(lexer);
        if (!tokens) {
            printf("Error: Tokenization failed\n");
            lexer_destroy(lexer);
            continue;
        }
        
        // Create parser
        Parser *parser = parser_create(tokens, lexer->token_count);
        if (!parser) {
            printf("Error: Failed to create parser\n");
            lexer_destroy(lexer);
            continue;
        }
        
        // Parse
        ASTNode *ast = parser_parse(parser);
        if (!ast) {
            printf("Error: Parsing failed\n");
            parser_destroy(parser);
            lexer_destroy(lexer);
            continue;
        }
        
        // Process imports if any
        if (ast->type == AST_PROGRAM) {
            for (int i = 0; i < ast->program.statement_count; i++) {
                if (ast->program.statements[i]->type == AST_IMPORT_STATEMENT) {
                    import_process_statement(import_manager, interpreter, 
                                           ast->program.statements[i]);
                }
            }
        }
        
        // Execute
        Value *result = interpreter_evaluate(interpreter, ast);
        if (result && result->type != VALUE_NULL) {
            printf("=> ");
            value_print(result);
            printf("\n");
            value_destroy(result);
        }
        
        // Cleanup
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        
        line_number++;
    }
    
    printf("\nGoodbye!\n");
    
    import_manager_destroy(import_manager);
    interpreter_destroy(interpreter);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        interactive_mode();
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive_mode();
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (!execute_file(argv[i])) {
                return 1;
            }
            return 0;
        }
    }
    
    return 0;
}