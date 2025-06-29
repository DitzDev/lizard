#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"

typedef enum {
    ERROR_LEXER,
    ERROR_PARSER,
    ERROR_RUNTIME,
    ERROR_IMPORT,
    ERROR_TYPE
} ErrorType;

typedef struct {
    ErrorType type;
    Position pos;
    char *message;
    char *suggestion;
    char *code_snippet;
} Error;

// Basic error reporting functions
void error_report(ErrorType type, Position pos, const char *message, const char *suggestion);
void error_report_with_code(ErrorType type, Position pos, const char *message, 
                           const char *suggestion, const char *code_snippet);

// Enhanced error reporting with recovery
void error_report_with_recovery(ErrorType type, Position pos, const char *message, 
                               const char *suggestion, const char *recovery_hint);

// Utility functions
const char *error_type_to_string(ErrorType type);
void error_show_code_context(const char *filename, int line, int column);
void error_reset_state(void);
bool is_same_position(Position pos1, Position pos2);
void error_report_type_fatal(Position pos, const char *message, const char *suggestion);
void error_show_code_context_smart(const char *filename, int line, int column, ErrorType type);
int get_error_highlight_width(ErrorType type, const char *code_line, int column);

#endif