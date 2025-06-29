#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "value.h"

typedef enum {
    AST_PROGRAM,
    AST_VARIABLE_DECLARATION,
    AST_FUNCTION_DECLARATION,
    AST_RETURN_STATEMENT,
    AST_EXPRESSION_STATEMENT,
    AST_BLOCK_STATEMENT,
    AST_PRINT_STATEMENT,
    AST_FUNCTION_CALL,
    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_IDENTIFIER,
    AST_LITERAL,
    AST_FORMAT_STRING,
    AST_IMPORT_STATEMENT,
    AST_ASSIGNMENT_EXPRESSION
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    Position pos;
    union {
        struct {
            struct ASTNode **statements;
            int statement_count;
        } program;
        
        struct {
            char *name;
            char *var_type;
            struct ASTNode *initializer;
        } variable_declaration;
        
        struct {
            char *name;
            struct ASTNode *value;
        } assignment_expression;
        
        struct {
            char *name;
            char **param_names;
            char **param_types;
            int param_count;
            char *return_type;
            struct ASTNode *body;
            bool is_public;
        } function_declaration;
        
        struct {
            struct ASTNode *expression;
        } return_statement;
        
        struct {
            struct ASTNode *expression;
        } expression_statement;
        
        struct {
            struct ASTNode **statements;
            int statement_count;
        } block_statement;
        
        struct {
            struct ASTNode *expression;
            bool newline;
        } print_statement;
        
        struct {
            char *name;
            struct ASTNode **arguments;
            int argument_count;
        } function_call;
        
        struct {
            struct ASTNode *left;
            TokenType operator;
            struct ASTNode *right;
        } binary_expression;
        
        struct {
            TokenType operator;
            struct ASTNode *operand;
        } unary_expression;
        
        struct {
            char *name;
        } identifier;
        
        struct {
            Value *value;
        } literal;
        struct {
            char **names;
            int name_count;
            char *module_path;
            char **aliases;
        } string_literal;
        
        struct {
            char *template;
            struct ASTNode **expressions;
            int expression_count;
        } format_string;
        
        struct {
            char **names;
            int name_count;
            char *module_path;
            char **aliases;
        } import_statement;
    };
} ASTNode;

typedef struct {
    Token *tokens;
    size_t token_count;
    size_t current;
} Parser;

Parser *parser_create(Token *tokens, size_t token_count);
void parser_destroy(Parser *parser);
ASTNode *parser_parse(Parser *parser);
void ast_destroy(ASTNode *node);
void ast_print(ASTNode *node, int indent);

#endif