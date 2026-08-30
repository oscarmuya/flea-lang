#include "../include/interpreter.h"
#include "../include/arena.h"
#include "../include/errors.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/store.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This is our interpreter (for now) we just do the legacy tree walking and
// execute

Value eval(ASTNode *node, Environment *env, int block_id) {
  Value ret = (Value){.type = VAL_NIL};
  switch (node->type) {
  case NODE_PROGRAM: {
    // we get a global block_id
    int global_scope = get_block_id(env);
    for (size_t i = 0; i < (size_t)node->as.program.count; i++) {
      eval(node->as.program.statements[i], env, global_scope);
    }
    break;
  }
  case NODE_NUMBER: {
    ret = eval_value(node, env);
    break;
  }
  case NODE_STRING: {
    ret = eval_value(node, env);
    break;
  }
  case NODE_BOOLEAN: {
    ret = eval_value(node, env);
    break;
  }
  case NODE_IDENTIFIER: {
    ret = eval_value(node, env);
    break;
  }
  case NODE_UNARY: {
    ret = eval_expr(node, env, block_id);
    break;
  }
  case NODE_BINARY: {
    ret = eval_expr(node, env, block_id);
    break;
  }
    // INFO: My lang my choice i will evaluate an assign as an expression like C
  case NODE_ASSIGN: {
    ret = eval_expr(node, env, block_id);
    break;
  }
  case NODE_LET: {
    execute_stmt(node, env, block_id);
    break;
  }
  case NODE_BLOCK: {
    int block_scope = get_block_id(env);
    for (size_t i = 0; i < (size_t)node->as.block.count; i++) {
      eval(node->as.block.statements[i], env, block_scope);
    }
    remove_by_block(&env->store, block_scope);
    break;
  }
  case NODE_PRINT: {
    if (node->as.print.value == NULL)
      throw_interpreter_error("Error: can't print value of UNDEFINED");
    Value value = eval(node->as.print.value, env, block_id);
    if (value.type == VAL_NUMBER)
      printf("%d\n", value.as.integer);
    else if (value.type == VAL_STRING)
      printf("%s\n", value.as.string);
    else if (value.type == VAL_BOOL)
      printf("%s\n", value.as.boolean ? "true" : "false");
    else
      printf("NULL\n");
    break;
  }
  case NODE_IF: {
    execute_stmt(node, env, block_id);
    break;
  }
  case NODE_WHILE: {
    execute_stmt(node, env, block_id);
    break;
  }
  default:
    break;
  }
  return ret;
}
// NODE_BINARY NODE_UNARY NODE_ASSIGN
// evaluates binary and unary
// unary: op can be ! -
// unary: operand can be String, Identifer, Number
// binary: left , right = string , identifier, number
// binary: op + - * / == < > <= >=
Value eval_expr(ASTNode *node, Environment *env, int block_id) {
  Value value = (Value){.type = VAL_NIL};
  switch (node->type) {
  case NODE_BINARY: {
    Value left = eval(node->as.binary.left, env, block_id);
    Value right = eval(node->as.binary.right, env, block_id);

    if (left.type == VAL_NIL || right.type == VAL_NIL)
      throw_interpreter_error("Error: Cant operate on value NULL");

    if (left.type == VAL_NUMBER)
      value = operate_on_integers(&left, &right, node->as.binary.op);
    else if (left.type == VAL_STRING)
      value = operate_on_strings(&left, &right, node->as.binary.op, env);
    else if (left.type == VAL_BOOL)
      value = operate_on_bools(&left, &right, node->as.binary.op);
  } break;
  case NODE_UNARY: {
    Value operand = eval(node->as.unary.operand, env, block_id);
    if (node->as.unary.op == VALUE_EXCLAMATION) {
      if (operand.type == VAL_NUMBER) {
        throw_interpreter_error("Error: cannot perform operation on numbers");
      } else if (operand.type == VAL_STRING) {
        throw_interpreter_error("Error: cannot perform operation on string");
      } else if (operand.type == VAL_BOOL) {
        value.type = VAL_BOOL;
        value.as.boolean = operand.as.boolean == true ? false : true;
      }
    } else if (node->as.unary.op == VALUE_MINUS) {
      if (operand.type == VAL_NUMBER) {
        value.type = VAL_NUMBER;
        value.as.integer = operand.as.integer * -1;
      } else if (operand.type == VAL_STRING) {
        throw_interpreter_error("Error: cannot perform operation on string");
      } else if (operand.type == VAL_BOOL) {
        throw_interpreter_error("Error: cannot perform operation on booleans");
      }
    }
  } break;
  case NODE_ASSIGN: {
    // lets check if the value exists
    Value *identifer =
        get_value(env, node->as.assign.identifier->as.identifier.value);
    if (identifer == NULL)
      throw_interpreter_error("Error: variable of name (%s) does not exist",
                              node->as.assign.identifier->as.identifier.value);
    Value new_value = eval(node->as.assign.value, env, block_id);
    value = *update_value(env, node->as.assign.identifier->as.identifier.value,
                          new_value, block_id);

  } break;
  default:
    break;
  }

  return value;
}

void execute_stmt(ASTNode *node, Environment *env, int block_id) {
  switch (node->type) {
  // we initialize the value and pass to assign
  case NODE_LET: {
    Value *identifier = get_value_with_block_id(
        env, node->as.let.identifier->as.identifier.value, block_id);
    // if value is available we scream you cant reinitialize
    if (identifier != NULL)
      throw_interpreter_error(
          "Error: you cant re-initialize block scoped value");
    Value val = eval(node->as.let.value, env, block_id);
    create_value(env, node->as.assign.identifier->as.identifier.value, val,
                 block_id);

  } break;
  case NODE_IF: {
    Value condition = eval(node->as.if_stmt.condition, env, block_id);
    if (condition.type != VAL_BOOL)
      throw_interpreter_error(
          "Error: while condition does not evaluate to a boolean");
    if (condition.as.boolean == true) {
      eval(node->as.if_stmt.then_branch, env, block_id);
    } else {
      eval(node->as.if_stmt.else_branch, env, block_id);
    }
  } break;
  case NODE_WHILE: {
    Value condition = eval(node->as.while_stmt.condition, env, block_id);
    if (condition.type != VAL_BOOL)
      throw_interpreter_error(
          "Error: while condition does not evaluate to a boolean");
    while (condition.as.boolean == true) {
      eval(node->as.while_stmt.body, env, block_id);
      condition = eval(node->as.while_stmt.condition, env, block_id);
    }
  } break;
  default:
    break;
  }
}

// IDENTIFIER STRING NUMBER BOOLEAN
Value eval_value(ASTNode *node, Environment *env) {
  Value value = (Value){.type = VAL_NIL};
  switch (node->type) {
  case NODE_STRING: {
    value.type = VAL_STRING;
    value.as.string =
        arena_alloc(env->arena, strlen(node->as.string.value) + 1);
    sprintf(value.as.string, "%s", node->as.string.value);
  } break;
  case NODE_IDENTIFIER: {
    Value *res = get_value(env, node->as.identifier.value);
    if (res == NULL)
      throw_interpreter_error("Error: variable of name (%s) does not exist",
                              node->as.identifier.value);
    value.type = res->type;
    value.as = res->as;
  } break;
  case NODE_NUMBER: {
    value.type = VAL_NUMBER;
    value.as.integer = node->as.number.value;
  } break;
  case NODE_BOOLEAN: {
    value.type = VAL_BOOL;
    value.as.boolean = node->as.boolean.value;
  } break;
  default:
    break;
  }
  return value;
}

Value operate_on_integers(Value *left, Value *right, TokenType op) {
  Value value = (Value){.type = VAL_NIL};
  value.type = left->type;
  if (op == VALUE_MINUS) {
    value.as.integer = left->as.integer - right->as.integer;
  } else if (op == VALUE_PLUS) {
    value.as.integer = left->as.integer + right->as.integer;
  } else if (op == VALUE_ASTERIK) {
    value.as.integer = left->as.integer * right->as.integer;
  } else if (op == VALUE_FORWARD_SLASH) {
    value.as.integer = left->as.integer / right->as.integer;
  } else if (op == VALUE_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer == right->as.integer;
  } else if (op == VALUE_NOT_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer != right->as.integer;
  } else if (op == VALUE_LESS_THAN) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer < right->as.integer;
  } else if (op == VALUE_LESS_THAN_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer <= right->as.integer;
  } else if (op == VALUE_GREATER_THAN) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer > right->as.integer;
  } else if (op == VALUE_GREATER_THAN_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.integer >= right->as.integer;
  } else {
    throw_interpreter_error("Error: integer operation %s not allowed",
                            token_type_to_string(op));
  }
  return value;
}

Value operate_on_strings(Value *left, Value *right, TokenType op,
                         Environment *env) {
  Value value = (Value){.type = VAL_NIL};
  value.type = left->type;
  if (op == VALUE_PLUS) {
    value.as.string = arena_alloc(env->arena, strlen(left->as.string) +
                                                  strlen(right->as.string) + 1);
    sprintf(value.as.string, "%s%s", left->as.string, right->as.string);
  } else if (op == VALUE_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = strcmp(left->as.string, right->as.string) == 0;
  } else if (op == VALUE_NOT_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = strcmp(left->as.string, right->as.string) != 0;
  } else {
    throw_interpreter_error("Error: string operation %s not allowed",
                            token_type_to_string(op));
  }
  return value;
}

Value operate_on_bools(Value *left, Value *right, TokenType op) {
  Value value = (Value){.type = VAL_NIL};
  value.type = left->type;
  if (op == VALUE_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.boolean == right->as.boolean;
  } else if (op == VALUE_NOT_EQUALS) {
    value.type = VAL_BOOL;
    value.as.boolean = left->as.boolean == right->as.boolean;
  } else {
    throw_interpreter_error("Error: boolean operation %s not allowed",
                            token_type_to_string(op));
  }
  return value;
}

Value *get_value(Environment *env, char *name) {
  Node *res = find_by_name(env->store, name);
  if (res == NULL)
    return NULL;

  return &res->value;
}
Value *get_value_with_block_id(Environment *env, char *name, int block_id) {
  Node *res = find_by_name_and_block(env->store, name, block_id);
  if (res == NULL)
    return NULL;

  return &res->value;
}
Value *create_value(Environment *env, char *name, Value value, int block_id) {
  add_value(&env->store, name, block_id, value);
  return &env->store->value;
}
Value *update_value(Environment *env, char *name, Value value, int block_id) {
  // first we first try to update it in current block
  Node *res = find_by_name_and_block(env->store, name, block_id);
  // if not there we try the nearest declaration
  if (res == NULL)
    res = find_by_name(env->store, name);
  // then we create it if not found
  if (res == NULL) {
    add_value(&env->store, name, block_id, value);
    return &env->store->value;
  }

  res->value.as = value.as;
  res->value.type = value.type;

  return &res->value;
}
void delete_value(Environment *env, char *name) {
  remove_by_name(&env->store, name);
}

int get_block_id(Environment *env) { return (env->next_id)++; }
