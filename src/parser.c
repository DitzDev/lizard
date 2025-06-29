#include "parser.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 100
#define MAX_PARAMS 100
#define MAX_STATEMENTS 1000
#define MAX_IMPORTS 100
#define MAX_FORMAT_EXPRESSIONS 1000

Parser *parser_create(Token *tokens, size_t token_count) {
  Parser *parser = malloc(sizeof(Parser));
  if (!parser)
    return NULL;

  parser->tokens = tokens;
  parser->token_count = token_count;
  parser->current = 0;
  return parser;
}

void parser_destroy(Parser *parser) {
  if (parser) {
    free(parser);
  }
}

static Token *parser_current_token(Parser *parser) {
  if (parser->current >= parser->token_count) {
    return &parser->tokens[parser->token_count - 1]; // EOF token
  }
  return &parser->tokens[parser->current];
}

static Token *parser_peek_token(Parser *parser) {
  if (parser->current + 1 >= parser->token_count) {
    return &parser->tokens[parser->token_count - 1];
  }
  return &parser->tokens[parser->current + 1];
}

static void parser_advance(Parser *parser) {
  if (parser->current < parser->token_count - 1) {
    parser->current++;
  }
}

static bool parser_match(Parser *parser, TokenType type) {
  if (parser_current_token(parser)->type == type) {
    parser_advance(parser);
    return true;
  }
  return false;
}

static bool parser_expect(Parser *parser, TokenType type, const char *message) {
  if (parser_current_token(parser)->type == type) {
    parser_advance(parser);
    return true;
  }

  error_report(
      ERROR_PARSER, parser_current_token(parser)->pos, message,
      "Check your syntax and ensure all tokens are properly formatted");
  parser_advance(parser);
  return false;
}

static ASTNode *ast_create_node(ASTNodeType type, Position pos) {
  ASTNode *node = malloc(sizeof(ASTNode));
  if (!node)
    return NULL;

  memset(node, 0, sizeof(ASTNode)); // Initialize all fields to 0/NULL
  node->type = type;
  node->pos = pos;

  // Safe copy of filename if it exists
  if (pos.filename && strlen(pos.filename) > 0) {
    node->pos.filename = strdup(pos.filename);
  } else {
    node->pos.filename = NULL;
  }

  return node;
}

static ASTNode *parser_parse_format_string(Parser *parser,
                                           const char *template) {
  ASTNode *node =
      ast_create_node(AST_FORMAT_STRING, parser_current_token(parser)->pos);
  if (!node)
    return NULL;

  node->format_string.template = strdup(template);
  if (!node->format_string.template) {
    ast_destroy(node);
    return NULL;
  }

  node->format_string.expressions =
      malloc(sizeof(ASTNode *) * MAX_FORMAT_EXPRESSIONS);
  if (!node->format_string.expressions) {
    ast_destroy(node);
    return NULL;
  }

  node->format_string.expression_count = 0;

  char *str = strdup(template);
  if (!str) {
    ast_destroy(node);
    return NULL;
  }

  char *pos = str;

  while ((pos = strchr(pos, '{')) != NULL &&
         node->format_string.expression_count < MAX_FORMAT_EXPRESSIONS) {
    char *end = strchr(pos, '}');
    if (end) {
      *end = '\0';
      char *var_name = pos + 1;

      ASTNode *var_node =
          ast_create_node(AST_IDENTIFIER, parser_current_token(parser)->pos);
      if (!var_node) {
        free(str);
        ast_destroy(node);
        return NULL;
      }

      var_node->identifier.name = strdup(var_name);
      if (!var_node->identifier.name) {
        ast_destroy(var_node);
        free(str);
        ast_destroy(node);
        return NULL;
      }

      node->format_string.expressions[node->format_string.expression_count++] =
          var_node;
      pos = end + 1;
    } else {
      break;
    }
  }

  free(str);
  return node;
}

static ASTNode *parser_process_string_literal(Parser *parser, const char *str_value, Position pos) {
  if (str_value && strchr(str_value, '$') && strchr(str_value, '{') && strchr(str_value, '}')) {
    return parser_parse_format_string(parser, str_value);
  } else {
    ASTNode *node = ast_create_node(AST_LITERAL, pos);
    if (!node) return NULL;
    node->literal.value = value_create_string(str_value);
    return node;
  }
}

static ASTNode *parser_parse_expression(Parser *parser);
static ASTNode *parser_parse_statement(Parser *parser);

static ASTNode *parser_parse_primary(Parser *parser) {
  Token *token = parser_current_token(parser);

  if (token->type == TOKEN_NUMBER) {
    parser_advance(parser);
    ASTNode *node = ast_create_node(AST_LITERAL, token->pos);
    if (!node)
      return NULL;

    if (strchr(token->value, '.')) {
      node->literal.value = value_create_float(atof(token->value));
    } else {
      node->literal.value = value_create_int(atoi(token->value));
    }
    return node;
  }

  if (token->type == TOKEN_STRING) {
    parser_advance(parser);
    return parser_process_string_literal(parser, token->value, token->pos);
  }

  if (token->type == TOKEN_IDENTIFIER) {
    parser_advance(parser);

    if (parser_current_token(parser)->type == TOKEN_LPAREN) {
      parser_advance(parser); // consume '('

      ASTNode *node = ast_create_node(AST_FUNCTION_CALL, token->pos);
      if (!node)
        return NULL;

      node->function_call.name = strdup(token->value);
      if (!node->function_call.name) {
        ast_destroy(node);
        return NULL;
      }

      node->function_call.arguments = malloc(sizeof(ASTNode *) * 100);
      if (!node->function_call.arguments) {
        ast_destroy(node);
        return NULL;
      }

      node->function_call.argument_count = 0;

      if (parser_current_token(parser)->type != TOKEN_RPAREN) {
        do {
          ASTNode *arg = parser_parse_expression(parser);
          if (!arg) {
            ast_destroy(node);
            return NULL;
          }

          node->function_call.arguments[node->function_call.argument_count++] =
              arg;
        } while (parser_match(parser, TOKEN_COMMA));
      }

      if (!parser_expect(parser, TOKEN_RPAREN,
                         "Expected ')' after function arguments")) {
        ast_destroy(node);
        return NULL;
      }
      return node;
    }

    ASTNode *node = ast_create_node(AST_IDENTIFIER, token->pos);
    if (!node)
      return NULL;

    node->identifier.name = strdup(token->value);
    if (!node->identifier.name) {
      ast_destroy(node);
      return NULL;
    }
    return node;
  }

  if (parser_match(parser, TOKEN_LPAREN)) {
    ASTNode *expr = parser_parse_expression(parser);
    if (!expr)
      return NULL;

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after expression")) {
      ast_destroy(expr);
      return NULL;
    }
    return expr;
  }

  error_report(ERROR_PARSER, token->pos, "Unexpected token in expression",
               "Expected a number, string, identifier, or closing '(' and ')'");
  parser_advance(parser);
  return NULL;
}

static ASTNode *parser_parse_print_statement(Parser *parser) {
  Token *token = parser_current_token(parser);
  bool newline = (token->type == TOKEN_PRINTLN);
  parser_advance(parser);

  if (!parser_expect(parser, TOKEN_LPAREN,
                     "Expected '(' after print/println")) {
    return NULL;
  }

  ASTNode *node = ast_create_node(AST_PRINT_STATEMENT, token->pos);
  if (!node)
    return NULL;

  node->print_statement.newline = newline;

  ASTNode *expr = parser_parse_expression(parser);
  if (!expr) {
    ast_destroy(node);
    return NULL;
  }

  node->print_statement.expression = expr;

  if (!parser_expect(parser, TOKEN_RPAREN,
                     "Expected ')' after print expression")) {
    ast_destroy(node);
    return NULL;
  }

  parser_match(parser, TOKEN_SEMICOLON);
  return node;
}

static ASTNode *parser_parse_unary(Parser *parser) {
  Token *token = parser_current_token(parser);

  if (token->type == TOKEN_MINUS || token->type == TOKEN_PLUS) {
    parser_advance(parser);
    ASTNode *node = ast_create_node(AST_UNARY_EXPRESSION, token->pos);
    if (!node)
      return NULL;

    node->unary_expression.operator = token->type;
    node->unary_expression.operand = parser_parse_unary(parser);

    if (!node->unary_expression.operand) {
      ast_destroy(node);
      return NULL;
    }
    return node;
  }

  return parser_parse_primary(parser);
}

static ASTNode *parser_parse_multiplicative(Parser *parser) {
  ASTNode *left = parser_parse_unary(parser);
  if (!left)
    return NULL;

  while (parser_current_token(parser)->type == TOKEN_MULTIPLY ||
         parser_current_token(parser)->type == TOKEN_DIVIDE) {
    Token *op = parser_current_token(parser);
    parser_advance(parser);

    ASTNode *node = ast_create_node(AST_BINARY_EXPRESSION, op->pos);
    if (!node) {
      ast_destroy(left);
      return NULL;
    }

    node->binary_expression.left = left;
    node->binary_expression.operator = op->type;
    node->binary_expression.right = parser_parse_unary(parser);

    if (!node->binary_expression.right) {
      ast_destroy(node);
      return NULL;
    }

    left = node;
  }

  return left;
}

static ASTNode *parser_parse_additive(Parser *parser) {
  ASTNode *left = parser_parse_multiplicative(parser);
  if (!left)
    return NULL;

  while (parser_current_token(parser)->type == TOKEN_PLUS ||
         parser_current_token(parser)->type == TOKEN_MINUS) {
    Token *op = parser_current_token(parser);
    parser_advance(parser);

    ASTNode *node = ast_create_node(AST_BINARY_EXPRESSION, op->pos);
    if (!node) {
      ast_destroy(left);
      return NULL;
    }

    node->binary_expression.left = left;
    node->binary_expression.operator = op->type;
    node->binary_expression.right = parser_parse_multiplicative(parser);

    if (!node->binary_expression.right) {
      ast_destroy(node);
      return NULL;
    }

    left = node;
  }

  return left;
}

static ASTNode *parser_parse_expression(Parser *parser) {
  return parser_parse_additive(parser);
}

static ASTNode *parser_parse_variable_declaration(Parser *parser) {
  Token *let_token = parser_current_token(parser);
  parser_advance(parser); // consume 'let'

  ASTNode *node = ast_create_node(AST_VARIABLE_DECLARATION, let_token->pos);

  // Check for type annotation
  if (parser_current_token(parser)->type == TOKEN_IDENTIFIER &&
      parser_peek_token(parser)->type == TOKEN_COLON) {
    // let type: name = value
    node->variable_declaration.var_type =
        strdup(parser_current_token(parser)->value);
    parser_advance(parser); // consume type
    parser_advance(parser); // consume ':'

    if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
      error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                   "Expected variable name after type annotation",
                   "Use format: let type: name = value");
      parser_advance(parser);
      return NULL;
    }

    node->variable_declaration.name =
        strdup(parser_current_token(parser)->value);
    parser_advance(parser);
  } else if (parser_current_token(parser)->type == TOKEN_IDENTIFIER) {
    // let name = value (auto type)
    node->variable_declaration.name =
        strdup(parser_current_token(parser)->value);
    node->variable_declaration.var_type = NULL;
    parser_advance(parser);
  } else {
    error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                 "Expected variable name or type annotation",
                 "Use 'let name = value' or 'let type: name = value'");
    parser_advance(parser);
    return NULL;
  }

  parser_expect(parser, TOKEN_ASSIGN, "Expected '=' after variable name");
  node->variable_declaration.initializer = parser_parse_expression(parser);
  parser_match(parser, TOKEN_SEMICOLON);

  return node;
}

static ASTNode *parser_parse_block_statement(Parser *parser) {
  Token *token = parser_current_token(parser);
  ASTNode *node = ast_create_node(AST_BLOCK_STATEMENT, token->pos);
  if (!node)
    return NULL;

  node->block_statement.statements = malloc(sizeof(ASTNode *) * 100);
  if (!node->block_statement.statements) {
    ast_destroy(node);
    return NULL;
  }
  node->block_statement.statement_count = 0;

  while (parser_current_token(parser)->type != TOKEN_RBRACE &&
         parser_current_token(parser)->type != TOKEN_EOF) {
    ASTNode *stmt = parser_parse_statement(parser);
    if (stmt) {
      node->block_statement
          .statements[node->block_statement.statement_count++] = stmt;
    } else {
      parser_advance(parser);
    }
  }

  if (!parser_expect(parser, TOKEN_RBRACE, "Expected '}' after block")) {
    ast_destroy(node);
    return NULL;
  }
  return node;
}

static ASTNode *parser_parse_function_declaration(Parser *parser) {
  Token *fnc_token = parser_current_token(parser);
  bool is_public = false;

  if (fnc_token->type == TOKEN_KEYWORD_PUB) {
    is_public = true;
    parser_advance(parser);
    parser_expect(parser, TOKEN_KEYWORD_FNC, "Expected 'fnc' after 'pub'");
  } else {
    parser_advance(parser); // consume 'fnc'
  }

  ASTNode *node = ast_create_node(AST_FUNCTION_DECLARATION, fnc_token->pos);
  if (!node)
    return NULL;

  node->function_declaration.is_public = is_public;

  if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
    error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                 "Expected function name", "Functions must have a name");
    ast_destroy(node);
    return NULL;
  }

  node->function_declaration.name = strdup(parser_current_token(parser)->value);
  if (!node->function_declaration.name) {
    ast_destroy(node);
    return NULL;
  }
  parser_advance(parser);

  if (!parser_expect(parser, TOKEN_LPAREN,
                     "Expected '(' after function name")) {
    ast_destroy(node);
    return NULL;
  }

  node->function_declaration.param_names = malloc(sizeof(char *) * 100);
  node->function_declaration.param_types = malloc(sizeof(char *) * 100);
  if (!node->function_declaration.param_names ||
      !node->function_declaration.param_types) {
    ast_destroy(node);
    return NULL;
  }

  node->function_declaration.param_count = 0;

  if (parser_current_token(parser)->type != TOKEN_RPAREN) {
    do {
      if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
        error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                     "Expected parameter type", "Use format: type name");
        ast_destroy(node);
        return NULL;
      }

      char *param_type = strdup(parser_current_token(parser)->value);
      if (!param_type) {
        ast_destroy(node);
        return NULL;
      }
      parser_advance(parser);

      if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
        error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                     "Expected parameter name after type",
                     "Use format: type name");
        free(param_type);
        ast_destroy(node);
        return NULL;
      }

      char *param_name = strdup(parser_current_token(parser)->value);
      if (!param_name) {
        free(param_type);
        ast_destroy(node);
        return NULL;
      }
      parser_advance(parser);

      node->function_declaration
          .param_types[node->function_declaration.param_count] = param_type;
      node->function_declaration
          .param_names[node->function_declaration.param_count] = param_name;
      node->function_declaration.param_count++;

    } while (parser_match(parser, TOKEN_COMMA));
  }

  if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after parameters")) {
    ast_destroy(node);
    return NULL;
  }

  if (parser_match(parser, TOKEN_ARROW)) {
    // Position arrow_pos = parser_current_token(parser)->pos;
    if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
      error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                   "Expected return type after '->'",
                   "Specify return type or remove '->'");
      ast_destroy(node);
      return NULL;
    }
    node->function_declaration.return_type =
        strdup(parser_current_token(parser)->value);
    // node->function_declaration.return_type_pos = arrow_pos;
    if (!node->function_declaration.return_type) {
      ast_destroy(node);
      return NULL;
    }
    parser_advance(parser);
  } else {
    node->function_declaration.return_type = NULL;
  }

  if (!parser_expect(parser, TOKEN_LBRACE,
                     "Expected '{' before function body")) {
    ast_destroy(node);
    return NULL;
  }

  node->function_declaration.body = parser_parse_block_statement(parser);
  if (!node->function_declaration.body) {
    ast_destroy(node);
    return NULL;
  }

  return node;
}

static ASTNode *parser_parse_return_statement(Parser *parser) {
  Token *token = parser_current_token(parser);
  parser_advance(parser); // consume 'return'

  ASTNode *node = ast_create_node(AST_RETURN_STATEMENT, token->pos);

  if (parser_current_token(parser)->type != TOKEN_SEMICOLON &&
      parser_current_token(parser)->type != TOKEN_RBRACE) {
    node->return_statement.expression = parser_parse_expression(parser);
  } else {
    node->return_statement.expression = NULL;
  }

  parser_match(parser, TOKEN_SEMICOLON);
  return node;
}

static ASTNode *parser_parse_assignment_or_expression(Parser *parser) {
  if (parser_current_token(parser)->type == TOKEN_IDENTIFIER) {
    Token *id_token = parser_current_token(parser);

    if (parser_peek_token(parser)->type == TOKEN_ASSIGN) {
      parser_advance(parser); // skip identifier
      parser_advance(parser); // skip '='

      ASTNode *assignment =
          ast_create_node(AST_ASSIGNMENT_EXPRESSION, id_token->pos);
      assignment->assignment_expression.name = strdup(id_token->value);
      assignment->assignment_expression.value = parser_parse_expression(parser);

      return assignment;
    }
  }

  return parser_parse_expression(parser);
}

static ASTNode *parser_parse_import_statement(Parser *parser) {
  Token *token = parser_current_token(parser);
  parser_advance(parser); // consume 'import'

  ASTNode *node = ast_create_node(AST_IMPORT_STATEMENT, token->pos);

  parser_expect(parser, TOKEN_LBRACE, "Expected '{' after import");

  // Parse import list
  node->import_statement.names = malloc(sizeof(char *) * 10);
  node->import_statement.aliases = malloc(sizeof(char *) * 10);
  node->import_statement.name_count = 0;

  do {
    if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
      error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                   "Expected identifier in import list",
                   "Import specific function names");
      parser_advance(parser);
      return NULL;
    }

    char *name = strdup(parser_current_token(parser)->value);
    parser_advance(parser);

    char *alias = NULL;
    if (parser_match(parser, TOKEN_KEYWORD_AS)) {
      if (parser_current_token(parser)->type != TOKEN_IDENTIFIER) {
        error_report(ERROR_PARSER, parser_current_token(parser)->pos,
                     "Expected alias name after 'as'", "Provide an alias name");
        free(name);
        parser_advance(parser);
        return NULL;
      }
      alias = strdup(parser_current_token(parser)->value);
      parser_advance(parser);
    }

    node->import_statement.names[node->import_statement.name_count] = name;
    node->import_statement.aliases[node->import_statement.name_count] = alias;
    node->import_statement.name_count++;

  } while (parser_match(parser, TOKEN_COMMA));

  parser_expect(parser, TOKEN_RBRACE, "Expected '}' after import list");

  parser_match(parser, TOKEN_SEMICOLON);
  return node;
}

static ASTNode *parser_parse_statement(Parser *parser) {
  Token *token = parser_current_token(parser);

  switch (token->type) {
  case TOKEN_KEYWORD_LET:
    return parser_parse_variable_declaration(parser);
  case TOKEN_KEYWORD_PUB:
  case TOKEN_KEYWORD_FNC:
    return parser_parse_function_declaration(parser);
  case TOKEN_KEYWORD_RETURN:
    return parser_parse_return_statement(parser);
  case TOKEN_KEYWORD_IMPORT:
    return parser_parse_import_statement(parser);
  case TOKEN_PRINT:
  case TOKEN_PRINTLN:
    return parser_parse_print_statement(parser);
  case TOKEN_LBRACE:
    parser_advance(parser);
    return parser_parse_block_statement(parser);
  case TOKEN_IDENTIFIER: {
    if (parser_peek_token(parser)->type == TOKEN_ASSIGN) {
      ASTNode *assignment = parser_parse_assignment_or_expression(parser);
      parser_match(parser, TOKEN_SEMICOLON);
      return assignment;
    }
    ASTNode *expr = parser_parse_expression(parser);
    if (expr) {
      ASTNode *stmt = ast_create_node(AST_EXPRESSION_STATEMENT, token->pos);
      stmt->expression_statement.expression = expr;
      parser_match(parser, TOKEN_SEMICOLON);
      return stmt;
    }
    return NULL;
  }
  default: {
    // Expression statement
    ASTNode *expr = parser_parse_expression(parser);
    if (expr) {
      ASTNode *stmt = ast_create_node(AST_EXPRESSION_STATEMENT, token->pos);
      stmt->expression_statement.expression = expr;
      parser_match(parser, TOKEN_SEMICOLON);
      return stmt;
    }
    return NULL;
  }
  }
}

ASTNode *parser_parse(Parser *parser) {
  ASTNode *program = ast_create_node(AST_PROGRAM, position_create(1, 1, ""));
  program->program.statements = malloc(sizeof(ASTNode *) * 100);
  program->program.statement_count = 0;

  while (parser_current_token(parser)->type != TOKEN_EOF) {
    ASTNode *stmt = parser_parse_statement(parser);
    if (stmt) {
      program->program.statements[program->program.statement_count++] = stmt;
    } else {
      // Skip to next statement on error
      while (parser_current_token(parser)->type != TOKEN_SEMICOLON &&
             parser_current_token(parser)->type != TOKEN_EOF &&
             parser_current_token(parser)->type != TOKEN_RBRACE) {
        parser_advance(parser);
      }
      if (parser_current_token(parser)->type == TOKEN_SEMICOLON) {
        parser_advance(parser);
      }
    }
  }

  return program;
}

void ast_destroy(ASTNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case AST_PROGRAM:
    for (int i = 0; i < node->program.statement_count; i++) {
      ast_destroy(node->program.statements[i]);
    }
    free(node->program.statements);
    break;
  case AST_VARIABLE_DECLARATION:
    free(node->variable_declaration.name);
    free(node->variable_declaration.var_type);
    ast_destroy(node->variable_declaration.initializer);
    break;
  case AST_FUNCTION_DECLARATION:
    free(node->function_declaration.name);
    for (int i = 0; i < node->function_declaration.param_count; i++) {
      free(node->function_declaration.param_names[i]);
      free(node->function_declaration.param_types[i]);
    }
    free(node->function_declaration.param_names);
    free(node->function_declaration.param_types);
    free(node->function_declaration.return_type);
    ast_destroy(node->function_declaration.body);
    break;
  case AST_RETURN_STATEMENT:
    ast_destroy(node->return_statement.expression);
    break;
  case AST_EXPRESSION_STATEMENT:
    ast_destroy(node->expression_statement.expression);
    break;
  case AST_BLOCK_STATEMENT:
    for (int i = 0; i < node->block_statement.statement_count; i++) {
      ast_destroy(node->block_statement.statements[i]);
    }
    free(node->block_statement.statements);
    break;
  case AST_PRINT_STATEMENT:
    ast_destroy(node->print_statement.expression);
    break;
  case AST_FUNCTION_CALL:
    free(node->function_call.name);
    for (int i = 0; i < node->function_call.argument_count; i++) {
      ast_destroy(node->function_call.arguments[i]);
    }
    free(node->function_call.arguments);
    break;
  case AST_BINARY_EXPRESSION:
    ast_destroy(node->binary_expression.left);
    ast_destroy(node->binary_expression.right);
    break;
  case AST_UNARY_EXPRESSION:
    ast_destroy(node->unary_expression.operand);
    break;
  case AST_IDENTIFIER:
    free(node->identifier.name);
    break;
  case AST_LITERAL:
    value_destroy(node->literal.value);
    break;
  case AST_FORMAT_STRING:
    free(node->format_string.template);
    for (int i = 0; i < node->format_string.expression_count; i++) {
      ast_destroy(node->format_string.expressions[i]);
    }
    free(node->format_string.expressions);
    break;
  case AST_ASSIGNMENT_EXPRESSION:
    free(node->assignment_expression.name);
    ast_destroy(node->assignment_expression.value);
    break;
  case AST_IMPORT_STATEMENT:
    for (int i = 0; i < node->import_statement.name_count; i++) {
      free(node->import_statement.names[i]);
      free(node->import_statement.aliases[i]);
    }
    free(node->import_statement.names);
    free(node->import_statement.aliases);
    free(node->import_statement.module_path);
    break;
  }

  free(node->pos.filename);
  free(node);
}

void ast_print(ASTNode *node, int indent) {
  if (!node)
    return;

  for (int i = 0; i < indent; i++)
    printf("  ");

  switch (node->type) {
  case AST_PROGRAM:
    printf("Program (%d statements)\n", node->program.statement_count);
    for (int i = 0; i < node->program.statement_count; i++) {
      ast_print(node->program.statements[i], indent + 1);
    }
    break;
  case AST_VARIABLE_DECLARATION:
    printf("VarDecl: %s", node->variable_declaration.name);
    if (node->variable_declaration.var_type) {
      printf(" : %s", node->variable_declaration.var_type);
    }
    printf("\n");
    if (node->variable_declaration.initializer) {
      ast_print(node->variable_declaration.initializer, indent + 1);
    }
    break;
  case AST_FUNCTION_DECLARATION:
    printf("FuncDecl: %s", node->function_declaration.name);
    if (node->function_declaration.is_public)
      printf(" (public)");
    printf("\n");
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Parameters:\n");
    for (int i = 0; i < node->function_declaration.param_count; i++) {
      for (int j = 0; j < indent + 2; j++)
        printf("  ");
      printf("%s %s\n", node->function_declaration.param_types[i],
             node->function_declaration.param_names[i]);
    }
    if (node->function_declaration.return_type) {
      for (int i = 0; i < indent + 1; i++)
        printf("  ");
      printf("Returns: %s\n", node->function_declaration.return_type);
    }
    ast_print(node->function_declaration.body, indent + 1);
    break;
  case AST_RETURN_STATEMENT:
    printf("Return\n");
    if (node->return_statement.expression) {
      ast_print(node->return_statement.expression, indent + 1);
    }
    break;
  case AST_EXPRESSION_STATEMENT:
    printf("ExprStmt\n");
    ast_print(node->expression_statement.expression, indent + 1);
    break;
  case AST_BLOCK_STATEMENT:
    printf("Block (%d statements)\n", node->block_statement.statement_count);
    for (int i = 0; i < node->block_statement.statement_count; i++) {
      ast_print(node->block_statement.statements[i], indent + 1);
    }
    break;
  case AST_PRINT_STATEMENT:
    printf("Print%s\n", node->print_statement.newline ? "ln" : "");
    ast_print(node->print_statement.expression, indent + 1);
    break;
  case AST_FUNCTION_CALL:
    printf("Call: %s (%d args)\n", node->function_call.name,
           node->function_call.argument_count);
    for (int i = 0; i < node->function_call.argument_count; i++) {
      ast_print(node->function_call.arguments[i], indent + 1);
    }
    break;
  case AST_BINARY_EXPRESSION:
    printf("BinaryOp: %s\n",
           token_type_to_string(node->binary_expression.operator));
    ast_print(node->binary_expression.left, indent + 1);
    ast_print(node->binary_expression.right, indent + 1);
    break;
  case AST_UNARY_EXPRESSION:
    printf("UnaryOp: %s\n",
           token_type_to_string(node->unary_expression.operator));
    ast_print(node->unary_expression.operand, indent + 1);
    break;
  case AST_IDENTIFIER:
    printf("Identifier: %s\n", node->identifier.name);
    break;
  case AST_LITERAL:
    printf("Literal: ");
    value_print(node->literal.value);
    printf("\n");
    break;
  case AST_FORMAT_STRING:
    printf("FormatString: %s (%d expressions)\n", node->format_string.template,
           node->format_string.expression_count);
    for (int i = 0; i < node->format_string.expression_count; i++) {
      ast_print(node->format_string.expressions[i], indent + 1);
    }
    break;
  case AST_ASSIGNMENT_EXPRESSION:
    printf("Assignment: %s\n", node->assignment_expression.name);
    ast_print(node->assignment_expression.value, indent + 1);
    break;
  case AST_IMPORT_STATEMENT:
    printf("Import: %d items\n", node->import_statement.name_count);
    for (int i = 0; i < node->import_statement.name_count; i++) {
      for (int j = 0; j < indent + 1; j++)
        printf("  ");
      printf("%s", node->import_statement.names[i]);
      if (node->import_statement.aliases[i]) {
        printf(" as %s", node->import_statement.aliases[i]);
      }
      printf("\n");
    }
    break;
  }
}
