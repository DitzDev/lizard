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

int get_error_highlight_width(ErrorType type, const char *code_line, int column) {
    if (!code_line || column < 1) return 1;
    
    int line_len = strlen(code_line);
    if (column > line_len) return 1;
    
    switch (type) {
        case ERROR_LEXER: {
            char current_char = code_line[column - 1];
            
            if (current_char == '%' && column < line_len && code_line[column] == '%') {
                return 2; // %% 
            }
            if (current_char == '=' && column < line_len && code_line[column] == '=') {
                return 2; // ==
            }
            if (current_char == '!' && column < line_len && code_line[column] == '=') {
                return 2; // !=
            }
            if (current_char == '<' && column < line_len && code_line[column] == '=') {
                return 2; // <=
            }
            if (current_char == '>' && column < line_len && code_line[column] == '=') {
                return 2; // >=
            }
            if (current_char == '&' && column < line_len && code_line[column] == '&') {
                return 2; // &&
            }
            if (current_char == '|' && column < line_len && code_line[column] == '|') {
                return 2; // ||
            }
            
            return 1;
        }
        
        case ERROR_TYPE: {
            int start = column - 2 * 2;
            int end = start;
            
            while (end < line_len && 
                   (isalnum(code_line[end]) || code_line[end] == '_' || 
                    code_line[end] == '"' || code_line[end] == '\'' || 
                    code_line[end] == '.')) {
                end++;
            }
            
            if (start < line_len && code_line[start] == '"') {
                while (end < line_len && code_line[end] != '"') {
                    end++;
                }
                if (end < line_len) end++;
            }
            
            return (end - start) > 0 ? (end - start) : 2;
        }
        
        case ERROR_PARSER: {
            int start = column - 1;
            int end = start;
            
            while (start > 0 && isspace(code_line[start - 1])) {
                start--;
            }
            
            while (end < line_len && !isspace(code_line[end]) && 
                   code_line[end] != ';' && code_line[end] != ',' && 
                   code_line[end] != '(' && code_line[end] != ')') {
                end++;
            }
            
            return (end - start) > 0 ? (end - start) : 1;
        }
        
        default:
            return 1;
    }
}

void error_show_code_context(const char *filename, int line, int column) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    
    char buffer[1024];
    int current_line = 1;
    
    while (fgets(buffer, sizeof(buffer), file) && current_line < line) {
        current_line++;
    }
    
    if (current_line == line) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        printf("   %d | %s\n", line, buffer);
        
        int line_prefix_width = 3; // "   "
        int line_num_width = snprintf(NULL, 0, "%d", line);
        line_prefix_width += line_num_width + 3;
        
        for (int i = 0; i < line_prefix_width; i++) {
            printf(" ");
        }
        
        for (int i = 1; i < column; i++) {
            printf(" ");
        }
        printf("^ here\n");
    }
    
    fclose(file);
}

void error_show_code_context_smart(const char *filename, int line, int column, ErrorType type) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    
    char buffer[1024];
    int current_line = 1;
    
    while (fgets(buffer, sizeof(buffer), file) && current_line < line) {
        current_line++;
    }
    
    if (current_line == line) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        printf("   %d | %s\n", line, buffer);
        
        int line_prefix_width = 3;
        int line_num_width = snprintf(NULL, 0, "%d", line);
        line_prefix_width += line_num_width + 3;
        
        for (int i = 0; i < line_prefix_width; i++) {
            printf(" ");
        }
        
        for (int i = 1; i < column; i++) {
            printf(" ");
        }
        
        int highlight_width = get_error_highlight_width(type, buffer, column);
        
        for (int i = 0; i < highlight_width; i++) {
            printf("^");
        }
        printf(" here\n");
    }
    
    fclose(file);
}

void error_report(ErrorType type, Position pos, const char *message, const char *suggestion) {
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return;
    }
    
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(type), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mError:\033[0m %s\n", message);
    
    error_show_code_context_smart(pos.filename, pos.line, pos.column, type);
    
    if (suggestion) {
        printf("   \033[1;36mNote:\033[0m %s\n", suggestion);
    }
    
    printf("\n");
    
    error_reported_at_position = true;
    last_error_pos = pos;
    
    if (type == ERROR_TYPE) {
        printf("   \033[1;31mType checking failed. Compilation terminated.\033[0m\n");
        printf("   \033[1;33mExiting with status %d\033[0m\n", EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
}

void error_report_with_code(ErrorType type, Position pos, const char *message, 
                           const char *suggestion, const char *code_snippet) {
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return;
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
        printf("   \033[1;36mNote:\033[0m %s\n", suggestion);
    }
    
    printf("\n");
    
    error_reported_at_position = true;
    last_error_pos = pos;
    
    if (type == ERROR_TYPE) {
        printf("   \033[1;31mType checking failed. Compilation terminated.\033[0m\n");
        printf("   \033[1;33mExiting with status %d\033[0m\n", EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
}

void error_report_with_recovery(ErrorType type, Position pos, const char *message, 
                               const char *suggestion, const char *recovery_hint) {
    if (error_reported_at_position && is_same_position(pos, last_error_pos)) {
        return;
    }
    
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(type), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mError:\033[0m %s\n", message);
    
    error_show_code_context_smart(pos.filename, pos.line, pos.column, type);
    
    if (suggestion) {
        printf("   \033[1;36mNote:\033[0m %s\n", suggestion);
    }
    
    if (recovery_hint) {
        printf("   \033[1;33mRecovery:\033[0m %s\n", recovery_hint);
    }
    
    printf("\n");
    
    error_reported_at_position = true;
    last_error_pos = pos;
    
    if (type == ERROR_TYPE) {
        printf("   \033[1;31mType checking failed. Compilation terminated.\033[0m\n");
        printf("   \033[1;33mExiting with status %d\033[0m\n", EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
}

void error_report_type_fatal(Position pos, const char *message, const char *suggestion) {
    printf("\n🦎 \033[1;31m%s\033[0m in \033[1m%s:%d:%d\033[0m\n", 
           error_type_to_string(ERROR_TYPE), pos.filename, pos.line, pos.column);
    
    printf("   \033[1;31mFatal Error:\033[0m %s\n", message);
    
    error_show_code_context_smart(pos.filename, pos.line, pos.column, ERROR_TYPE);
    
    if (suggestion) {
        printf("   \033[1;36mNote:\033[0m %s\n", suggestion);
    }
    
    printf("   \033[1;31mType checking failed. Compilation terminated.\033[0m\n");
    printf("   \033[1;33mExiting with status %d\033[0m\n\n", EXIT_FAILURE);
    exit(EXIT_FAILURE);
}

void error_reset_state(void) {
    error_reported_at_position = false;
    last_error_pos.line = 0;
    last_error_pos.column = 0;
    last_error_pos.filename = NULL;
}