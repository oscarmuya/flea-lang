#include "../include/parser.h"
#include "../include/arena.h"
#include "../include/errors.h"
#include "../include/lexer.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tokens parsing
// This is where we create our AST  WARN: (Yikes!!)

const char *node_type_to_string(NodeType type);

ASTNode *parse_program(Parser *parser) {
  ASTNode **statements = NULL;
  int count = 0;
  int capacity = 0;

  while (!is_at_end(parser)) {
    ASTNode *stmt = parse_statement(parser);

    if (capacity == count) {
      size_t old_size = sizeof(ASTNode *) * (size_t)capacity;
      capacity = capacity == 0 ? 8 : capacity * 2;
      statements = arena_realloc(parser->arena, statements, old_size,
                                 sizeof(ASTNode *) * (size_t)capacity);
    }
    statements[count++] = stmt;
  }

  ASTNode *program = arena_alloc(parser->arena, sizeof(*program));
  program->type = NODE_PROGRAM;
  program->as.program.statements = statements;
  program->as.program.count = count;
  return program;
}

ASTNode *parse_statement(Parser *parser) {
  ASTNode *stmt = NULL;

  stmt = parse_print_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_make_fn_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_call_fn_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_make_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_expr_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_should_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_whilst_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_comment(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_block(parser);
  if (stmt != NULL)
    return stmt;

  return stmt;
}

// we actually dont have to do that much we dont even need the comment value for
// now
ASTNode *parse_comment(Parser *parser) {
  if (peek(parser)->type != VALUE_COMMENT)
    return NULL;
  advance(parser);
  ASTNode *comment = arena_alloc(parser->arena, sizeof(*comment));
  comment->type = NODE_COMMENT;
  return comment;
}

// printStmt -> "print" equality ";"
ASTNode *parse_print_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_PRINT)
    return NULL;
  advance(parser);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_PRINT;

  stmt->as.print.value = parse_equality(parser);

  // expect ;
  expect(parser, VALUE_SEMICOLON);
  return stmt;
}

// makeStmt -> "make" expression ";"
ASTNode *parse_make_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_MAKE)
    return NULL;
  advance(parser);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_MAKE;

  ASTNode *expression = parse_expression(parser);
  if (expression == NULL) {
    throw_parser_error("Error: expected expression found NULL");
  }

  stmt->as.make.identifier = expression->as.be.identifier;
  stmt->as.make.value = expression->as.be.value;

  // expect ;
  expect(parser, VALUE_SEMICOLON);
  return stmt;
}

// exprStmt -> expression ";"
ASTNode *parse_expr_stmt(Parser *parser) {
  ASTNode *stmt = parse_expression(parser);
  if (stmt == NULL)
    return NULL;

  // expect ;
  expect(parser, VALUE_SEMICOLON);
  return stmt;
}

// shouldStmt -> "should" "(" equality ")" block ("then" block)?
ASTNode *parse_should_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_SHOULD)
    return NULL;
  advance(parser);
  // expect (
  expect(parser, VALUE_LEFT_PAREN);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_SHOULD;

  ASTNode *eq = parse_equality(parser);
  if (eq == NULL)
    throw_parser_error("Error: expected equality in should statement");
  stmt->as.should_stmt.condition = eq;

  // expect )
  expect(parser, VALUE_RIGHT_PAREN);

  stmt->as.should_stmt.then_branch = parse_block(parser);

  // see if we have else
  if (peek(parser)->type == VALUE_THEN) {
    advance(parser);
    stmt->as.should_stmt.else_branch = parse_block(parser);
  }
  return stmt;
}

// whilstStmt -> "whilst" "(" equality ")" block
ASTNode *parse_whilst_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_WHILST)
    return NULL;
  advance(parser);
  expect(parser, VALUE_LEFT_PAREN);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_WHILST;

  ASTNode *eq = parse_equality(parser);
  if (eq == NULL)
    throw_parser_error("Error: expected equality in while statement");
  stmt->as.whilst_stmt.condition = eq;

  // expect )
  expect(parser, VALUE_RIGHT_PAREN);
  stmt->as.whilst_stmt.body = parse_block(parser);

  return stmt;
}

// makeFnStmt  ::= "make" IDENTIFER "(" paramList ")" block
ASTNode *parse_make_fn_stmt(Parser *parser) {
  // we look ahead and see if this is a function making
  if (peek(parser)->type != VALUE_MAKE || look_ahead(parser, 2) == NULL ||
      look_ahead(parser, 2)->type != VALUE_LEFT_PAREN)
    return NULL;
  advance(parser);
  ASTNode *identifier = parse_primary(parser);
  if (identifier == NULL)
    throw_parser_error("Error: function name is required");

  ASTNode *fn = arena_alloc(parser->arena, sizeof(*fn));
  fn->type = NODE_MAKE_FN;
  fn->as.make_fn_stmt.fn_name = identifier;

  expect(parser, VALUE_LEFT_PAREN);
  fn->as.make_fn_stmt.params = parse_param_list(parser);
  expect(parser, VALUE_RIGHT_PAREN);
  ASTNode *body = parse_block(parser);
  fn->as.make_fn_stmt.body = body;
  return fn;
}

// callFnStmt  ::= IDENTIFIER "(" argList ")"
ASTNode *parse_call_fn_stmt(Parser *parser) {
  // we lookahead for the paren
  if (peek(parser)->type != VALUE_IDENTIFIER || look_ahead(parser, 1) == NULL ||
      look_ahead(parser, 1)->type != VALUE_LEFT_PAREN)
    return NULL;
  ASTNode *identifier = parse_primary(parser);
  if (identifier == NULL)
    throw_parser_error("Error: function name is required");

  ASTNode *fn = arena_alloc(parser->arena, sizeof(*fn));
  fn->type = NODE_CALL_FN;
  fn->as.call_fn_stmt.fn_name = identifier;

  expect(parser, VALUE_LEFT_PAREN);
  fn->as.call_fn_stmt.args = parse_arg_list(parser);
  expect(parser, VALUE_RIGHT_PAREN);
  expect(parser, VALUE_SEMICOLON);
  return fn;
}

// paramList   ::= (IDENTIFER ("," IDENTIFER)*)?
ASTNode *parse_param_list(Parser *parser) {
  size_t count = 0;
  // INFO: dont know if im wasting memory here but whatever man
  size_t capacity = 8;

  ASTNode *params = arena_alloc(parser->arena, sizeof(*params));
  params->type = NODE_PARAMS;
  params->as.params.params =
      arena_alloc(parser->arena, capacity * sizeof(params->as.params.params));

  ASTNode *identifier = parse_primary(parser);
  if (identifier == NULL) {
    params->as.params.count = count;
    return params;
  }
  params->as.params.params[count++] = identifier;
  while (peek(parser)->type == VALUE_COMMA) {
    if (count >= capacity) {
      size_t prev = capacity * sizeof(params->as.params.params);
      capacity *= 2;
      params->as.params.params =
          arena_realloc(parser->arena, params->as.params.params, prev,
                        capacity * sizeof(params->as.params.params));
    }
    advance(parser);
    ASTNode *prim = parse_primary(parser);
    // if it is null they have added an extra comm
    if (prim == NULL)
      throw_parser_error("Error: extra comma found in function params");
    params->as.params.params[count++] = prim;
  }

  params->as.params.count = count;
  return params;
}

// argList     ::= (expression ("," expression)*)?
ASTNode *parse_arg_list(Parser *parser) {
  size_t count = 0;
  size_t capacity = 8;

  ASTNode *args = arena_alloc(parser->arena, sizeof(*args));
  args->as.args.args =
      arena_alloc(parser->arena, capacity * sizeof(args->as.args.args));
  args->type = NODE_ARGS;

  ASTNode *exp = parse_expression(parser);
  if (exp == NULL) {
    args->as.args.count = count;
    return args;
  }
  args->as.args.args[count++] = exp;
  while (peek(parser)->type == VALUE_COMMA) {
    if (count >= capacity) {
      size_t prev = capacity * sizeof(args->as.args.args);
      capacity *= 2;
      args->as.args.args =
          arena_realloc(parser->arena, args->as.args.args, prev,
                        capacity * sizeof(args->as.args.args));
    }
    advance(parser);
    exp = parse_expression(parser);
    // if it is null they have added an extra comm
    if (exp == NULL)
      throw_parser_error("Error: extra comma found in function args");
    args->as.args.args[count++] = exp;
  }
  args->as.args.count = count;
  return args;
}

// block -> "{" statement* "}"
ASTNode *parse_block(Parser *parser) {
  if (peek(parser)->type != VALUE_LEFT_BRACE)
    return NULL;
  advance(parser);

  ASTNode *block = arena_alloc(parser->arena, sizeof(*block));
  block->type = NODE_BLOCK;

  // loop and parse statements
  ASTNode **statements = NULL;
  int count = 0;
  int capacity = 0;

  while (peek(parser)->type != VALUE_RIGHT_BRACE && !is_at_end(parser)) {
    ASTNode *stmt = parse_statement(parser);

    if (capacity == count) {
      size_t old_size = sizeof(ASTNode *) * (size_t)capacity;
      capacity = capacity == 0 ? 8 : capacity * 2;
      statements = arena_realloc(parser->arena, statements, old_size,
                                 sizeof(ASTNode *) * (size_t)capacity);
    }
    statements[count++] = stmt;
  }

  block->as.block.statements = statements;
  block->as.block.count = count;

  // expect }
  expect(parser, VALUE_RIGHT_BRACE);
  return block;
}

// expression -> assignment
ASTNode *parse_expression(Parser *parser) { return parse_assignment(parser); }

// assignment -> IDENTIFIER "=" assignment | equality
ASTNode *parse_assignment(Parser *parser) {
  if (peek(parser)->type == VALUE_IDENTIFIER) {
    if (look_ahead(parser, 1)->type != VALUE_BE) {
      return parse_equality(parser);
    }
    Token *identifier = advance(parser);
    ASTNode *assgn = arena_alloc(parser->arena, sizeof(*assgn));

    assgn->type = NODE_BE;
    assgn->as.be.identifier = arena_alloc(parser->arena, sizeof(ASTNode));
    assgn->as.be.identifier->type = NODE_IDENTIFIER;

    char *s = identifier->value.s;
    size_t len = strlen(s);
    assgn->as.be.identifier->as.identifier.value =
        arena_alloc(parser->arena, len + 1);
    memcpy(assgn->as.be.identifier->as.identifier.value, s, len);
    assgn->as.be.identifier->as.identifier.value[len] = '\0';

    expect(parser, VALUE_BE);
    ASTNode *value = parse_assignment(parser);
    assgn->as.be.value = value;

    return assgn;
  }
  return parse_equality(parser);
}

// equality -> comparison (("==" | "!=") comparison)?
ASTNode *parse_equality(Parser *parser) {
  ASTNode *left = parse_comparison(parser);
  if (left == NULL)
    return NULL;

  ASTNode *eq = NULL;

  switch (peek(parser)->type) {
  case VALUE_EQUALS:
    eq = arena_alloc(parser->arena, sizeof(*eq));
    eq->type = NODE_BINARY;
    eq->as.binary.op = VALUE_EQUALS;
    break;
  case VALUE_NOT_EQUALS:
    eq = arena_alloc(parser->arena, sizeof(*eq));
    eq->type = NODE_BINARY;
    eq->as.binary.op = VALUE_NOT_EQUALS;
    break;
  default:
    break;
  }
  if (eq != NULL) {
    advance(parser);
    ASTNode *right = parse_comparison(parser);
    if (right == NULL)
      throw_parser_error("Error: right comparison needed for equality");
    eq->as.binary.left = left;
    eq->as.binary.right = right;
    return eq;
  }
  return left;
}

// comparison -> term (("<" | ">" | "<=" | ">=") term)?
ASTNode *parse_comparison(Parser *parser) {
  ASTNode *left = parse_term(parser);
  if (left == NULL)
    return NULL;

  ASTNode *comp = NULL;

  switch (peek(parser)->type) {
  case VALUE_LESS_THAN:
    comp = arena_alloc(parser->arena, sizeof(*comp));
    comp->type = NODE_BINARY;
    comp->as.binary.op = VALUE_LESS_THAN;
    break;
  case VALUE_GREATER_THAN:
    comp = arena_alloc(parser->arena, sizeof(*comp));
    comp->type = NODE_BINARY;
    comp->as.binary.op = VALUE_GREATER_THAN;
    break;
  case VALUE_GREATER_THAN_EQUALS:
    comp = arena_alloc(parser->arena, sizeof(*comp));
    comp->type = NODE_BINARY;
    comp->as.binary.op = VALUE_GREATER_THAN_EQUALS;
    break;
  case VALUE_LESS_THAN_EQUALS:
    comp = arena_alloc(parser->arena, sizeof(*comp));
    comp->type = NODE_BINARY;
    comp->as.binary.op = VALUE_LESS_THAN_EQUALS;
    break;
  default:
    break;
  }
  if (comp != NULL) {
    advance(parser);
    ASTNode *right = parse_term(parser);
    if (right == NULL)
      throw_parser_error("Error: right term needed for comparison");
    comp->as.binary.left = left;
    comp->as.binary.right = right;
    return comp;
  }
  return left;
}

// term -> factor (("+" | "-") factor)*
ASTNode *parse_term(Parser *parser) {
  ASTNode *left = parse_factor(parser);
  if (left == NULL)
    return NULL;

  TokenType type = peek(parser)->type;
  while (type == VALUE_PLUS || type == VALUE_MINUS) {
    ASTNode *term = arena_alloc(parser->arena, sizeof(*term));
    term->type = NODE_BINARY;
    term->as.binary.op = type;
    advance(parser);
    ASTNode *right = parse_factor(parser);
    if (right == NULL)
      throw_parser_error("Error: right factor needed for term");
    term->as.binary.left = left;
    term->as.binary.right = right;
    left = term;
    type = peek(parser)->type;
  }
  return left;
}

// factor -> unary (("*" | "/") unary)*
ASTNode *parse_factor(Parser *parser) {
  ASTNode *left = parse_unary(parser);
  if (left == NULL)
    return NULL;

  TokenType type = peek(parser)->type;
  while (type == VALUE_ASTERIK || type == VALUE_FORWARD_SLASH) {
    ASTNode *factor = arena_alloc(parser->arena, sizeof(*factor));
    factor->type = NODE_BINARY;
    factor->as.binary.op = type;
    advance(parser);
    ASTNode *right = parse_unary(parser);
    if (right == NULL)
      throw_parser_error("Error: right unary needed for factor");
    factor->as.binary.left = left;
    factor->as.binary.right = right;
    left = factor;
    type = peek(parser)->type;
  }
  return left;
}

// unary -> ("!" | "-") unary | primary
ASTNode *parse_unary(Parser *parser) {
  if (peek(parser)->type == VALUE_EXCLAMATION ||
      peek(parser)->type == VALUE_MINUS) {

    ASTNode *unary = arena_alloc(parser->arena, sizeof(*unary));
    unary->type = NODE_UNARY;
    unary->as.unary.op = advance(parser)->type;
    unary->as.unary.operand = parse_unary(parser);
    if (unary->as.unary.operand == NULL)
      throw_parser_error("Error: expected operand found NULL");
    return unary;
  }
  return parse_primary(parser);
}

// primary ::= NUMBER | STRING | BOOL | IDENTIFIER | "(" expression ")"
ASTNode *parse_primary(Parser *parser) {
  ASTNode *primary = NULL;

  switch (peek(parser)->type) {
  case VALUE_NUMBER:
    primary = arena_alloc(parser->arena, sizeof(*primary));
    primary->type = NODE_NUMBER;
    primary->as.number.value = advance(parser)->value.i;
    break;
  case VALUE_STRING:
    primary = arena_alloc(parser->arena, sizeof(*primary));
    primary->type = NODE_STRING;
    char *s = advance(parser)->value.s;
    size_t len = strlen(s);
    primary->as.string.value = arena_alloc(parser->arena, len + 1);
    memcpy(primary->as.string.value, s, len);
    primary->as.string.value[len] = '\0';
    break;
  case VALUE_BOOL:
    primary = arena_alloc(parser->arena, sizeof(*primary));
    primary->type = NODE_BOOLEAN;
    primary->as.boolean.value = advance(parser)->value.b;
    break;
  case VALUE_IDENTIFIER:
    primary = arena_alloc(parser->arena, sizeof(*primary));
    primary->type = NODE_IDENTIFIER;
    char *s1 = advance(parser)->value.s;
    size_t len1 = strlen(s1);
    primary->as.string.value = arena_alloc(parser->arena, len1 + 1);
    memcpy(primary->as.string.value, s1, len1);
    primary->as.string.value[len1] = '\0';
    break;
  case VALUE_LEFT_PAREN:
    advance(parser);
    primary = parse_expression(parser);
    expect(parser, VALUE_RIGHT_PAREN);
    break;
  default:
    break;
  }

  return primary;
}

Token *advance(Parser *parser) { return &parser->tokens[(parser->current)++]; }
Token *expect(Parser *parser, TokenType expected) {
  if (parser->tokens[parser->current].type == expected) {
    return advance(parser);
  }
  throw_parser_error(
      "Error: expected %s but found %s!\n", token_type_to_string(expected),
      token_type_to_string(parser->tokens[parser->current].type));
}
Token *peek(Parser *parser) { return &parser->tokens[parser->current]; }
Token *look_ahead(Parser *parser, size_t steps) {
  if ((size_t)parser->current + steps >= (size_t)parser->total)
    return NULL;
  return &parser->tokens[(size_t)parser->current + steps];
}
bool is_at_end(Parser *parser) { return parser->current >= parser->total; }

const char *node_type_to_string(NodeType type) {
  switch (type) {
  case NODE_PROGRAM:
    return "NODE_PROGRAM";
  case NODE_NUMBER:
    return "NODE_NUMBER";
  case NODE_STRING:
    return "NODE_STRING";
  case NODE_BOOLEAN:
    return "NODE_BOOLEAN";
  case NODE_IDENTIFIER:
    return "NODE_IDENTIFIER";
  case NODE_UNARY:
    return "NODE_UNARY";
  case NODE_BINARY:
    return "NODE_BINARY";
  case NODE_BE:
    return "NODE_BE";
  case NODE_MAKE:
    return "NODE_MAKE";
  case NODE_BLOCK:
    return "NODE_BLOCK";
  case NODE_PRINT:
    return "NODE_PRINT";
  case NODE_SHOULD:
    return "NODE_SHOULD";
  case NODE_WHILST:
    return "NODE_WHILST";
  default:
    return "INVALID_NODE_TYPE";
  }
}
