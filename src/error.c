#include "error.h"

// Global error state to prevent spam
static bool error_reported_at_position = false;
static Position last_error_pos = {0, 0, NULL};

const char *error_type_to_string(ErrorType type) {
    switch (type) {
        case ERROR_LEXER: return "Lexer Error";
        case ERROR_PARSER: return "Parser Error";
        case ERROR_RUNTIME: return "Runtime Error";
        case ERROR_IMPORT: return "Import Error";
        case ERROR_TYPE: return "Type Error";
        default: return "Unknown Error";
    }
}

bool is_same_position(Position pos1, Position pos2) {
    return pos1.line == pos2.line && 
           pos1.column == pos2.column && 
           pos1.filename && pos2.filename &&
           strcmp(pos1.filename, pos2.filename) == 0;
}

void error_show_code_context(const char *filename, int line, int column) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    
    char buffer[1024];
    int current_line = 1;
    
    // Find the error line
    while (fgets(buffer, sizeof(buffer), file) && current_line < line) {
        current_line++;
    }
    
    if (current_line == line) {
        // Remove newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        printf("   %d | %s\n", line, buffer);
        printf("     | ");
        
        // Add spaces to align with error column
        for (int i = 1; i < column; i++) {
            printf(" ");
        }
        printf("^ here\n");
    }
    
    fclose(file);
}

void error_report(ErrorType type, Position pos, const char *message, const char *suggestion) {
    // Prevent spam: check if we already reported error at this exact position
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return; // Don't report the same error again
    }
    
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(type), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mError:\033[0m %s\n", message);
    
    error_show_code_context(pos.filename, pos.line, pos.column);
    
    if (suggestion) {
        printf("   \033[1;36mSuggestion:\033[0m %s\n", suggestion);
    }
    
    printf("\n");
    
    // Mark this position as reported
    error_reported_at_position = true;
    last_error_pos = pos;
}

void error_report_with_code(ErrorType type, Position pos, const char *message, 
                           const char *suggestion, const char *code_snippet) {
    // Prevent spam: check if we already reported error at this exact position
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return; // Don't report the same error again
    }
    
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(type), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mError:\033[0m %s\n", message);
    
    if (code_snippet) {
        printf("   %d | %s\n", pos.line, code_snippet);
        printf("     | ");
        for (int i = 1; i < pos.column; i++) {
            printf(" ");
        }
        printf("^ here\n");
    }
    
    if (suggestion) {
        printf("   \033[1;36mSuggestion:\033[0m %s\n", suggestion);
    }
    
    printf("\n");
    
    // Mark this position as reported
    error_reported_at_position = true;
    last_error_pos = pos;
}

// Call this function to reset error state (e.g., when starting to parse a new file)
void error_reset_state(void) {
    error_reported_at_position = false;
    last_error_pos.line = 0;
    last_error_pos.column = 0;
    last_error_pos.filename = NULL;
}

// Enhanced error reporting with recovery suggestions
void error_report_with_recovery(ErrorType type, Position pos, const char *message, 
                               const char *suggestion, const char *recovery_hint) {
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return;
    }
    
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(type), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mError:\033[0m %s\n", message);
    
    error_show_code_context(pos.filename, pos.line, pos.column);
    
    if (suggestion) {
        printf("   \033[1;36mSuggestion:\033[0m %s\n", suggestion);
    }
    
    if (recovery_hint) {
        printf("   \033[1;33mRecovery:\033[0m %s\n", recovery_hint);
    }
    
    printf("\n");
    
    error_reported_at_position = true;
    last_error_pos = pos;
}