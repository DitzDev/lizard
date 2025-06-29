#include "lexer.h"
#include "error.h"

Lexer *lexer_create(const char *source, const char *filename) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->source = strdup(source);
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = strdup(filename);
    lexer->tokens = malloc(sizeof(Token) * 100);
    lexer->token_count = 0;
    lexer->token_capacity = 100;
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    
    free(lexer->source);
    free(lexer->filename);
    
    for (size_t i = 0; i < lexer->token_count; i++) {
        free(lexer->tokens[i].value);
        free(lexer->tokens[i].pos.filename);
    }
    free(lexer->tokens);
    free(lexer);
}

Position position_create(int line, int column, char *filename) {
    Position pos;
    pos.line = line;
    pos.column = column;
    pos.filename = strdup(filename);
    return pos;
}

static char lexer_current_char(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->source[lexer->pos];
}

static char lexer_peek_char(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->pos + 1];
}

static void lexer_advance(Lexer *lexer) {
    if (lexer->pos < lexer->length) {
        if (lexer->source[lexer->pos] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->pos++;
    }
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer_current_char(lexer))) {
        lexer_advance(lexer);
    }
}

static void lexer_add_token(Lexer *lexer, TokenType type, const char *value) {
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity *= 2;
        lexer->tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
    }
    
    Token *token = &lexer->tokens[lexer->token_count++];
    token->type = type;
    token->value = strdup(value);
    token->pos = position_create(lexer->line, lexer->column, lexer->filename);
}

static char *lexer_read_string(Lexer *lexer) {
    char quote = lexer_current_char(lexer);
    lexer_advance(lexer); // Skip opening quote
    
    char *buffer = malloc(256);
    size_t capacity = 256;
    size_t length = 0;
    
    while (lexer_current_char(lexer) != quote && lexer_current_char(lexer) != '\0') {
        if (length >= capacity - 1) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        
        if (lexer_current_char(lexer) == '\\') {
            lexer_advance(lexer);
            switch (lexer_current_char(lexer)) {
                case 'n': buffer[length++] = '\n'; break;
                case 't': buffer[length++] = '\t'; break;
                case 'r': buffer[length++] = '\r'; break;
                case '\\': buffer[length++] = '\\'; break;
                case '"': buffer[length++] = '"'; break;
                case '\'': buffer[length++] = '\''; break;
                default: buffer[length++] = lexer_current_char(lexer); break;
            }
        } else {
            buffer[length++] = lexer_current_char(lexer);
        }
        lexer_advance(lexer);
    }
    
    if (lexer_current_char(lexer) != quote) {
        error_report(ERROR_LEXER, position_create(lexer->line, lexer->column, lexer->filename),
                    "Unterminated string literal", "Add closing quote");
        free(buffer);
        return NULL;
    }
    
    lexer_advance(lexer); // Skip closing quote
    buffer[length] = '\0';
    return buffer;
}

static char *lexer_read_number(Lexer *lexer) {
    char *buffer = malloc(64);
    size_t length = 0;
    bool has_dot = false;
    
    while (isdigit(lexer_current_char(lexer)) || 
           (lexer_current_char(lexer) == '.' && !has_dot)) {
        if (lexer_current_char(lexer) == '.') {
            has_dot = true;
        }
        buffer[length++] = lexer_current_char(lexer);
        lexer_advance(lexer);
    }
    
    buffer[length] = '\0';
    return buffer;
}

static char *lexer_read_identifier(Lexer *lexer) {
    char *buffer = malloc(128);
    size_t length = 0;
    
    while (isalnum(lexer_current_char(lexer)) || lexer_current_char(lexer) == '_') {
        buffer[length++] = lexer_current_char(lexer);
        lexer_advance(lexer);
    }
    
    buffer[length] = '\0';
    return buffer;
}

static TokenType lexer_keyword_or_identifier(const char *text) {
    if (strcmp(text, "let") == 0) return TOKEN_KEYWORD_LET;
    if (strcmp(text, "fnc") == 0) return TOKEN_KEYWORD_FNC;
    if (strcmp(text, "return") == 0) return TOKEN_KEYWORD_RETURN;
    if (strcmp(text, "pub") == 0) return TOKEN_KEYWORD_PUB;
    if (strcmp(text, "import") == 0) return TOKEN_KEYWORD_IMPORT;
    if (strcmp(text, "as") == 0) return TOKEN_KEYWORD_AS;
    if (strcmp(text, "print") == 0) return TOKEN_PRINT;
    if (strcmp(text, "println") == 0) return TOKEN_PRINTLN;
    if (strcmp(text, "int") == 0) return TOKEN_IDENTIFIER;
    if (strcmp(text, "string") == 0) return TOKEN_IDENTIFIER;
    if (strcmp(text, "float") == 0) return TOKEN_IDENTIFIER;
    return TOKEN_IDENTIFIER;
}

static void lexer_skip_comment(Lexer *lexer) {
    if (lexer_current_char(lexer) == '#') {
        if (lexer_peek_char(lexer) == '#' && lexer->pos + 2 < lexer->length && 
            lexer->source[lexer->pos + 2] == '#') {
            // Multiline comment ###...###
            lexer_advance(lexer); // #
            lexer_advance(lexer); // #
            lexer_advance(lexer); // #
            
            while (lexer->pos + 2 < lexer->length) {
                if (lexer_current_char(lexer) == '#' && 
                    lexer_peek_char(lexer) == '#' && 
                    lexer->source[lexer->pos + 2] == '#') {
                    lexer_advance(lexer); // #
                    lexer_advance(lexer); // #
                    lexer_advance(lexer); // #
                    break;
                }
                lexer_advance(lexer);
            }
        } else {
            // Single line comment
            while (lexer_current_char(lexer) != '\n' && lexer_current_char(lexer) != '\0') {
                lexer_advance(lexer);
            }
        }
    }
}

Token *lexer_tokenize(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        lexer_skip_whitespace(lexer);
        
        char current = lexer_current_char(lexer);
        if (current == '\0') break;
        
        // Comments
        if (current == '#') {
            lexer_skip_comment(lexer);
            continue;
        }
        
        // String literals
        if (current == '"' || current == '\'') {
            char *str = lexer_read_string(lexer);
            if (str) {
                lexer_add_token(lexer, TOKEN_STRING, str);
                free(str);
            }
            continue;
        }
        
        // Numbers
        if (isdigit(current)) {
            char *num = lexer_read_number(lexer);
            lexer_add_token(lexer, TOKEN_NUMBER, num);
            free(num);
            continue;
        }
        
        // Identifiers and keywords
        if (isalpha(current) || current == '_') {
            char *ident = lexer_read_identifier(lexer);
            TokenType type = lexer_keyword_or_identifier(ident);
            lexer_add_token(lexer, type, ident);
            free(ident);
            continue;
        }
        
        // Two-character operators
        if (current == '-' && lexer_peek_char(lexer) == '>') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            lexer_add_token(lexer, TOKEN_ARROW, "->");
            continue;
        }
        
        if (current == '$' && lexer_peek_char(lexer) == '{') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            lexer_add_token(lexer, TOKEN_DOLLAR_LBRACE, "${");
            continue;
        }
        
        if (current == '%' && lexer_peek_char(lexer) == '%') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            lexer_add_token(lexer, TOKEN_INT_DIVIDE, "%%");
            continue;
        }
        
        // Single-character tokens
        switch (current) {
            case ':': lexer_add_token(lexer, TOKEN_COLON, ":"); break;
            case ';': lexer_add_token(lexer, TOKEN_SEMICOLON, ";"); break;
            case ',': lexer_add_token(lexer, TOKEN_COMMA, ","); break;
            case '.': lexer_add_token(lexer, TOKEN_DOT, "."); break;
            case '=': lexer_add_token(lexer, TOKEN_ASSIGN, "="); break;
            case '+': lexer_add_token(lexer, TOKEN_PLUS, "+"); break;
            case '-': lexer_add_token(lexer, TOKEN_MINUS, "-"); break;
            case '*': lexer_add_token(lexer, TOKEN_MULTIPLY, "*"); break;
            case '/': lexer_add_token(lexer, TOKEN_DIVIDE, "/"); break;
            case '%': lexer_add_token(lexer, TOKEN_MODULO, "%"); break;
            case '(': lexer_add_token(lexer, TOKEN_LPAREN, "("); break;
            case ')': lexer_add_token(lexer, TOKEN_RPAREN, ")"); break;
            case '{': lexer_add_token(lexer, TOKEN_LBRACE, "{"); break;
            case '}': lexer_add_token(lexer, TOKEN_RBRACE, "}"); break;
            case '[': lexer_add_token(lexer, TOKEN_LBRACKET, "["); break;
            case ']': lexer_add_token(lexer, TOKEN_RBRACKET, "]"); break;
            default:
                error_report(ERROR_LEXER, position_create(lexer->line, lexer->column, lexer->filename),
                           "Unexpected character", "Remove or escape this character");
                lexer_advance(lexer);
                continue;
        }
        lexer_advance(lexer);
    }
    
    lexer_add_token(lexer, TOKEN_EOF, "");
    return lexer->tokens;
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_KEYWORD_LET: return "LET";
        case TOKEN_KEYWORD_FNC: return "FNC";
        case TOKEN_KEYWORD_RETURN: return "RETURN";
        case TOKEN_KEYWORD_PUB: return "PUB";
        case TOKEN_KEYWORD_IMPORT: return "IMPORT";
        case TOKEN_KEYWORD_AS: return "AS";
        case TOKEN_PRINT: return "PRINT";
        case TOKEN_PRINTLN: return "PRINTLN";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULTIPLY: return "MULTIPLY";
        case TOKEN_DIVIDE: return "DIVIDE";
        case TOKEN_MODULO: return "MODULO";         
        case TOKEN_INT_DIVIDE: return "INT_DIVIDE";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_MULTILINE_COMMENT: return "MULTILINE_COMMENT";
        case TOKEN_FORMAT_STRING: return "FORMAT_STRING";
        case TOKEN_DOLLAR_LBRACE: return "DOLLAR_LBRACE";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void token_print(Token *token) {
    printf("Token: %s, Value: '%s', Line: %d, Column: %d\n",
           token_type_to_string(token->type), token->value, 
           token->pos.line, token->pos.column);
}