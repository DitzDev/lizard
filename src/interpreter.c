#include "interpreter.h"
#include "error.h"
#include "parser.h"

static bool is_compatible_type(Value *value, const char *expected_type) {
  if (!expected_type)
    return true; // No return type specified

  if (strcmp(expected_type, "int") == 0) {
    return value->type == VALUE_INT;
  } else if (strcmp(expected_type, "float") == 0) {
    return value->type == VALUE_FLOAT;
  } else if (strcmp(expected_type, "string") == 0) {
    return value->type == VALUE_STRING;
  } else if (strcmp(expected_type, "bool") == 0) {
    return value->type == VALUE_BOOL;
  } else if (strcmp(expected_type, "void") == 0) {
    return value->type == VALUE_NULL;
  }

  return false;
}

static const char *get_value_type_name(Value *value) {
  switch (value->type) {
  case VALUE_INT:
    return "int";
  case VALUE_FLOAT:
    return "float";
  case VALUE_STRING:
    return "string";
  case VALUE_BOOL:
    return "bool";
  case VALUE_NULL:
    return "void";
  case VALUE_FUNCTION:
    return "function";
  default:
    return "unknown";
  }
}

Interpreter *interpreter_create(void) {
  Interpreter *interpreter = malloc(sizeof(Interpreter));
  interpreter->global_env = environment_create(NULL);
  interpreter->current_env = interpreter->global_env;
  interpreter->return_flag = false;
  interpreter->return_value = NULL;
  return interpreter;
}

void interpreter_destroy(Interpreter *interpreter) {
  if (interpreter) {
    environment_destroy(interpreter->global_env);
    if (interpreter->return_value) {
      value_destroy(interpreter->return_value);
    }
    free(interpreter);
  }
}

static Value *evaluate_binary_expression(Interpreter *interpreter,
                                         ASTNode *node) {
  Value *left = interpreter_evaluate(interpreter, node->binary_expression.left);
  Value *right =
      interpreter_evaluate(interpreter, node->binary_expression.right);

  if (!left || !right) {
    if (left)
      value_destroy(left);
    if (right)
      value_destroy(right);
    return NULL;
  }

  Value *result = NULL;

  switch (node->binary_expression.operator) {
  case TOKEN_PLUS:
    if (left->type == VALUE_STRING || right->type == VALUE_STRING) {
      char *left_str = value_to_string(left);
      char *right_str = value_to_string(right);
      size_t len = strlen(left_str) + strlen(right_str) + 1;
      char *concat = malloc(len);
      strcpy(concat, left_str);
      strcat(concat, right_str);
      result = value_create_string(concat);
      free(concat);
      free(left_str);
      free(right_str);
    } else if (left->type == VALUE_INT && right->type == VALUE_INT) {
      result = value_create_int(left->int_val + right->int_val);
    } else if ((left->type == VALUE_INT || left->type == VALUE_FLOAT) &&
               (right->type == VALUE_INT || right->type == VALUE_FLOAT)) {
      double left_val =
          (left->type == VALUE_INT) ? left->int_val : left->float_val;
      double right_val =
          (right->type == VALUE_INT) ? right->int_val : right->float_val;
      result = value_create_float(left_val + right_val);
    }
    break;

  case TOKEN_MINUS:
    if (left->type == VALUE_INT && right->type == VALUE_INT) {
      result = value_create_int(left->int_val - right->int_val);
    } else if ((left->type == VALUE_INT || left->type == VALUE_FLOAT) &&
               (right->type == VALUE_INT || right->type == VALUE_FLOAT)) {
      double left_val =
          (left->type == VALUE_INT) ? left->int_val : left->float_val;
      double right_val =
          (right->type == VALUE_INT) ? right->int_val : right->float_val;
      result = value_create_float(left_val - right_val);
    }
    break;

  case TOKEN_MULTIPLY:
    if (left->type == VALUE_INT && right->type == VALUE_INT) {
      result = value_create_int(left->int_val * right->int_val);
    } else if ((left->type == VALUE_INT || left->type == VALUE_FLOAT) &&
               (right->type == VALUE_INT || right->type == VALUE_FLOAT)) {
      double left_val =
          (left->type == VALUE_INT) ? left->int_val : left->float_val;
      double right_val =
          (right->type == VALUE_INT) ? right->int_val : right->float_val;
      result = value_create_float(left_val * right_val);
    }
    break;

  case TOKEN_DIVIDE:
    if ((left->type == VALUE_INT || left->type == VALUE_FLOAT) &&
        (right->type == VALUE_INT || right->type == VALUE_FLOAT)) {
      double left_val =
          (left->type == VALUE_INT) ? left->int_val : left->float_val;
      double right_val =
          (right->type == VALUE_INT) ? right->int_val : right->float_val;
      if (right_val == 0) {
        error_report(ERROR_RUNTIME, node->pos, "Division by zero",
                     "Check the divisor value before performing division");
        value_destroy(left);
        value_destroy(right);
        return NULL;
      }
      result = value_create_float(left_val / right_val);
    }
    break;

  default:
    error_report(ERROR_RUNTIME, node->pos, "Unsupported binary operator",
                 "Use supported operators: +, -, *, /");
    break;
  }

  value_destroy(left);
  value_destroy(right);
  return result;
}

static Value *evaluate_unary_expression(Interpreter *interpreter,
                                        ASTNode *node) {
  Value *operand =
      interpreter_evaluate(interpreter, node->unary_expression.operand);
  if (!operand)
    return NULL;

  Value *result = NULL;

  switch (node->unary_expression.operator) {
  case TOKEN_MINUS:
    if (operand->type == VALUE_INT) {
      result = value_create_int(-operand->int_val);
    } else if (operand->type == VALUE_FLOAT) {
      result = value_create_float(-operand->float_val);
    } else {
      error_report(ERROR_RUNTIME, node->pos, "Cannot negate non-numeric value",
                   "Use unary minus only with numbers");
    }
    break;

  default:
    error_report(ERROR_RUNTIME, node->pos, "Unsupported unary operator",
                 "Use supported unary operators");
    break;
  }

  value_destroy(operand);
  return result;
}

static Value *evaluate_function_call(Interpreter *interpreter, ASTNode *node) {
  Value *func_value =
      environment_get(interpreter->current_env, node->function_call.name);
  if (!func_value || func_value->type != VALUE_FUNCTION) {
    error_report(ERROR_RUNTIME, node->pos, "Function not found or not callable",
                 "Check if the function is defined and accessible");
    return NULL;
  }

  Function *func = func_value->function_val;

  if (node->function_call.argument_count != func->param_count) {
    error_report(ERROR_RUNTIME, node->pos, "Argument count mismatch",
                 "Check the function signature and provide the correct number "
                 "of arguments");
    return NULL;
  }

  Environment *func_env = environment_create(interpreter->current_env);

  for (int i = 0; i < func->param_count; i++) {
    Value *arg_value =
        interpreter_evaluate(interpreter, node->function_call.arguments[i]);
    if (!arg_value) {
      environment_destroy(func_env);
      return NULL;
    }
    environment_define(func_env, func->param_names[i], arg_value,
                       func->param_types[i]);
  }

  Environment *prev_env = interpreter->current_env;
  interpreter->current_env = func_env;
  bool prev_return_flag = interpreter->return_flag;
  Value *prev_return_value = interpreter->return_value;

  interpreter->return_flag = false;
  interpreter->return_value = NULL;

  char *current_func_return_type = func->return_type;
  bool current_func_is_public = func->is_public;
  Position func_declaration_pos = func->declaration_pos;
  interpreter_evaluate(interpreter, func->body);

  Value *result = NULL;
  if (interpreter->return_flag && interpreter->return_value) {
    if (current_func_return_type &&
        !is_compatible_type(interpreter->return_value,
                            current_func_return_type)) {
      char error_msg[256];
      char suggestion[256];

      snprintf(error_msg, sizeof(error_msg),
               "Return type mismatch in function '%s': expected '%s', got '%s'",
               func->name, current_func_return_type,
               get_value_type_name(interpreter->return_value));

      if (current_func_is_public) {
        strcpy(suggestion,
               "Return type does not match the function's requirements");
      } else {
        snprintf(suggestion, sizeof(suggestion),
                 "Convert the return value to '%s' or change the function's "
                 "return type",
                 current_func_return_type);
      }

      error_report(ERROR_TYPE, func_declaration_pos, error_msg, suggestion);

      value_destroy(interpreter->return_value);
      interpreter->current_env = prev_env;
      interpreter->return_flag = prev_return_flag;
      interpreter->return_value = prev_return_value;
      environment_destroy(func_env);
      return NULL;
    }

    result = value_copy(interpreter->return_value);
    value_destroy(interpreter->return_value);
  } else {
    if (func->return_type && strcmp(func->return_type, "void") != 0) {
      char error_msg[256];
      snprintf(error_msg, sizeof(error_msg),
               "Function '%s' should return '%s' but no return statement found",
               func->name, func->return_type);

      error_report(ERROR_TYPE, func_declaration_pos, error_msg,
                   "Add a return statement with the correct type");

      interpreter->current_env = prev_env;
      interpreter->return_flag = prev_return_flag;
      interpreter->return_value = prev_return_value;
      environment_destroy(func_env);
      return NULL;
    }

    result = value_create_null();
  }

  interpreter->current_env = prev_env;
  interpreter->return_flag = prev_return_flag;
  interpreter->return_value = prev_return_value;

  environment_destroy(func_env);
  return result;
}

static Value *evaluate_format_string(Interpreter *interpreter, ASTNode *node) {
  char *result = malloc(1024);
  result[0] = '\0';

  const char *template = node->format_string.template;
  int expr_index = 0;
  size_t result_len = 0;
  size_t result_capacity = 1024;

  for (size_t i = 0; template[i]; i++) {
    if (template[i] == '$' && template[i + 1] == '{') {
      // Find the end of the placeholder
      size_t end = i + 2;
      while (template[end] && template[end] != '}')
        end++;

      if (template[end] == '}' &&
          expr_index < node->format_string.expression_count) {
        Value *expr_value = interpreter_evaluate(
            interpreter, node->format_string.expressions[expr_index]);
        if (!expr_value) {
           printf("LIZARD-INTERPRET-DEBUG: Failed to evaluate expression at index %d\n", expr_index);
        }
        if (expr_value) {
          char *expr_str = value_to_string(expr_value);
          size_t expr_len = strlen(expr_str);

          // Resize result buffer if needed
          while (result_len + expr_len >= result_capacity) {
            result_capacity *= 2;
            result = realloc(result, result_capacity);
          }

          strcat(result, expr_str);
          result_len += expr_len;

          free(expr_str);
          value_destroy(expr_value);
        } else {
           char placeholder[256];
          size_t placeholder_len __attribute__((unused)) = end - (i + 2);
          strncpy(placeholder, template + i, end - i + 1);
          placeholder[end - i + 1] = '\0';
          
          size_t ph_len = strlen(placeholder);
          while (result_len + ph_len >= result_capacity) {
            result_capacity *= 2;
            result = realloc(result, result_capacity);
          }
        }
        expr_index++;
        i = end;
      } else {
        // Invalid placeholder, treat as literal
        if (result_len + 1 >= result_capacity) {
          result_capacity *= 2;
          result = realloc(result, result_capacity);
        }
        result[result_len++] = template[i];
        result[result_len] = '\0';
      }
    } else {
      if (result_len + 1 >= result_capacity) {
        result_capacity *= 2;
        result = realloc(result, result_capacity);
      }
      result[result_len++] = template[i];
      result[result_len] = '\0';
    }
  }

  Value *string_value = value_create_string(result);
  free(result);
  return string_value;
}

Value *interpreter_evaluate(Interpreter *interpreter, ASTNode *node) {
  if (!node)
    return NULL;

  switch (node->type) {
  case AST_PROGRAM:
    for (int i = 0; i < node->program.statement_count; i++) {
      interpreter_evaluate(interpreter, node->program.statements[i]);
      if (interpreter->return_flag)
        break;
    }
    return NULL;

  case AST_VARIABLE_DECLARATION: {
    Value *value = NULL;
    if (node->variable_declaration.initializer) {
      value = interpreter_evaluate(interpreter,
                                   node->variable_declaration.initializer);
      if (!value)
        return NULL;
    } else {
      value = value_create_null();
    }

    if (!environment_define(interpreter->current_env,
                            node->variable_declaration.name, value,
                            node->variable_declaration.var_type)) {
      error_report(
          ERROR_RUNTIME, node->pos, "Variable already declared in this scope",
          "Use a different variable name or assign to existing variable");
      value_destroy(value);
      return NULL;
    }

    Value *result = value_copy(value);
    value_destroy(value);
    return result;
  }

  case AST_FUNCTION_DECLARATION: {
    Position return_type_pos = node->pos;
    if (node->function_declaration.return_type) {
      return_type_pos.column += strlen(node->function_declaration.name) + 10;
    }
    Function *func = function_create(
        node->function_declaration.name, node->function_declaration.param_names,
        node->function_declaration.param_types,
        node->function_declaration.param_count,
        node->function_declaration.return_type, node->function_declaration.body,
        node->function_declaration.is_public, return_type_pos);

    Value *func_value = value_create_function(func);
    environment_define(interpreter->current_env,
                       node->function_declaration.name, func_value, "function");
    return NULL;
  }

  case AST_RETURN_STATEMENT: {
    if (node->return_statement.expression) {
      interpreter->return_value =
          interpreter_evaluate(interpreter, node->return_statement.expression);
    } else {
      interpreter->return_value = value_create_null();
    }
    interpreter->return_flag = true;
    return NULL;
  }

  case AST_EXPRESSION_STATEMENT:
    return interpreter_evaluate(interpreter,
                                node->expression_statement.expression);

  case AST_BLOCK_STATEMENT: {
    Environment *block_env = environment_create(interpreter->current_env);
    Environment *prev_env = interpreter->current_env;
    interpreter->current_env = block_env;

    for (int i = 0; i < node->block_statement.statement_count; i++) {
      interpreter_evaluate(interpreter, node->block_statement.statements[i]);
      if (interpreter->return_flag)
        break;
    }

    interpreter->current_env = prev_env;
    environment_destroy(block_env);
    return NULL;
  }

  case AST_PRINT_STATEMENT: {
    Value *value =
        interpreter_evaluate(interpreter, node->print_statement.expression);
    if (value) {
      value_print(value);
      if (node->print_statement.newline) {
        printf("\n");
      }
      value_destroy(value);
    }
    return NULL;
  }

  case AST_FUNCTION_CALL:
    return evaluate_function_call(interpreter, node);

  case AST_BINARY_EXPRESSION:
    return evaluate_binary_expression(interpreter, node);

  case AST_UNARY_EXPRESSION:
    return evaluate_unary_expression(interpreter, node);

  case AST_IDENTIFIER: {
    Value *value =
        environment_get(interpreter->current_env, node->identifier.name);
    if (!value) {
      error_report(ERROR_RUNTIME, node->pos, "Undefined variable",
                   "Check if the variable is declared and in scope");
      return NULL;
    }
    return value_copy(value);
  }

  case AST_LITERAL:
    return value_copy(node->literal.value);

  case AST_FORMAT_STRING:
    return evaluate_format_string(interpreter, node);

  case AST_ASSIGNMENT_EXPRESSION: {
    Value *value =
        interpreter_evaluate(interpreter, node->assignment_expression.value);
    if (!value)
      return NULL;

    if (!environment_set(interpreter->current_env,
                         node->assignment_expression.name, value)) {
      error_report(ERROR_RUNTIME, node->pos, "Variable not declared",
                   "Declare the variable with 'let' before assignment");
      value_destroy(value);
      return NULL;
    }

    Value *result = value_copy(value);
    value_destroy(value);
    return result;
  }
  case AST_IMPORT_STATEMENT:
    // Import handling should be done before interpretation
    return NULL;

  default:
    error_report(ERROR_RUNTIME, node->pos, "Unknown AST node type",
                 "This might be a compiler bug");
    return NULL;
  }
}

void interpreter_run(Interpreter *interpreter, ASTNode *ast) {
  interpreter_evaluate(interpreter, ast);
}
