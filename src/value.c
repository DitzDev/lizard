#include "value.h"

Value *value_create_int(int val) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_INT;
  value->int_val = val;
  return value;
}

Value *value_create_float(double val) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_FLOAT;
  value->float_val = val;
  return value;
}

Value *value_create_string(const char *val) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_STRING;
  value->string_val = strdup(val);
  return value;
}

Value *value_create_bool(bool val) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_BOOL;
  value->bool_val = val;
  return value;
}

Value *value_create_function(Function *func) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_FUNCTION;
  value->function_val = func;
  return value;
}

Value *value_create_null(void) {
  Value *value = malloc(sizeof(Value));
  value->type = VALUE_NULL;
  return value;
}

void value_destroy(Value *value) {
  if (!value)
    return;

  switch (value->type) {
  case VALUE_STRING:
    free(value->string_val);
    break;
  case VALUE_FUNCTION:
    function_destroy(value->function_val);
    break;
  default:
    break;
  }
  free(value);
}

Value *value_copy(Value *value) {
  if (!value)
    return NULL;

  switch (value->type) {
  case VALUE_INT:
    return value_create_int(value->int_val);
  case VALUE_FLOAT:
    return value_create_float(value->float_val);
  case VALUE_STRING:
    return value_create_string(value->string_val);
  case VALUE_BOOL:
    return value_create_bool(value->bool_val);
  case VALUE_FUNCTION:
    return value_create_function(value->function_val);
  case VALUE_NULL:
    return value_create_null();
  default:
    return NULL;
  }
}

void value_print(Value *value) {
  if (!value) {
    printf("null");
    return;
  }

  switch (value->type) {
  case VALUE_INT:
    printf("%d", value->int_val);
    break;
  case VALUE_FLOAT:
    printf("%.2f", value->float_val);
    break;
  case VALUE_STRING:
    printf("%s", value->string_val);
    break;
  case VALUE_BOOL:
    printf("%s", value->bool_val ? "true" : "false");
    break;
  case VALUE_FUNCTION:
    printf("<function %s>", value->function_val->name);
    break;
  case VALUE_NULL:
    printf("null");
    break;
  }
}

char *value_to_string(Value *value) {
  if (!value)
    return strdup("null");

  char *result = malloc(256);
  switch (value->type) {
  case VALUE_INT:
    snprintf(result, 256, "%d", value->int_val);
    break;
  case VALUE_FLOAT:
    snprintf(result, 256, "%.2f", value->float_val);
    break;
  case VALUE_STRING:
    return strdup(value->string_val);
  case VALUE_BOOL:
    snprintf(result, 256, "%s", value->bool_val ? "true" : "false");
    break;
  case VALUE_FUNCTION:
    snprintf(result, 256, "<function %s>", value->function_val->name);
    break;
  case VALUE_NULL:
    snprintf(result, 256, "null");
    break;
  }
  return result;
}

const char *value_type_to_string(ValueType type) {
  switch (type) {
  case VALUE_INT:
    return "int";
  case VALUE_FLOAT:
    return "float";
  case VALUE_STRING:
    return "string";
  case VALUE_BOOL:
    return "bool";
  case VALUE_FUNCTION:
    return "function";
  case VALUE_NULL:
    return "null";
  default:
    return "unknown";
  }
}

Function *function_create(const char *name, char **param_names,
                          char **param_types, int param_count,
                          const char *return_type, struct ASTNode *body,
                          bool is_public, Position declaration_pos) {
  Function *func = malloc(sizeof(Function));
  func->name = strdup(name);
  func->param_count = param_count;
  func->body = body;
  func->is_public = is_public;
  func->declaration_pos = declaration_pos;

  if (param_count > 0) {
    func->param_names = malloc(sizeof(char *) * param_count);
    func->param_types = malloc(sizeof(char *) * param_count);
    for (int i = 0; i < param_count; i++) {
      func->param_names[i] = strdup(param_names[i]);
      func->param_types[i] = strdup(param_types[i]);
    }
  } else {
    func->param_names = NULL;
    func->param_types = NULL;
  }

  func->return_type = return_type ? strdup(return_type) : NULL;
  return func;
}

void function_destroy(Function *func) {
  if (!func)
    return;

  free(func->name);
  if (func->param_names) {
    for (int i = 0; i < func->param_count; i++) {
      free(func->param_names[i]);
      free(func->param_types[i]);
    }
    free(func->param_names);
    free(func->param_types);
  }
  free(func->return_type);
  free(func);
}
