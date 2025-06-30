#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TOKEN_EOF,
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_KEYWORD_LET,
  TOKEN_KEYWORD_FIXED,
  TOKEN_KEYWORD_FNC,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_PUB,
  TOKEN_KEYWORD_IMPORT,
  TOKEN_KEYWORD_AS,
  TOKEN_PRINT,
  TOKEN_PRINTLN,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_ARROW,
  TOKEN_ASSIGN,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_MODULO,        // %%
  TOKEN_INT_DIVIDE,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_COMMENT,
  TOKEN_MULTILINE_COMMENT,
  TOKEN_FORMAT_STRING,
  TOKEN_DOLLAR_LBRACE,
  TOKEN_ERROR
} TokenType;

typedef struct {
  int line;
  int column;
  char *filename;
} Position;

typedef struct {
  TokenType type;
  char *value;
  Position pos;
} Token;

typedef struct {
  char *source;
  size_t pos;
  size_t length;
  int line;
  int column;
  char *filename;
  Token *tokens;
  size_t token_count;
  size_t token_capacity;
} Lexer;

Lexer *lexer_create(const char *source, const char *filename);
void lexer_destroy(Lexer *lexer);
Token *lexer_tokenize(Lexer *lexer);
const char *token_type_to_string(TokenType type);
void token_print(Token *token);
Position position_create(int line, int column, char *filename);

#endif
