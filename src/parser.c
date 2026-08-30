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
  stmt = parse_let_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_expr_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_if_stmt(parser);
  if (stmt != NULL)
    return stmt;
  stmt = parse_while_stmt(parser);
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

// letStmt -> "let" expression ";"
ASTNode *parse_let_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_LET)
    return NULL;
  advance(parser);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_LET;

  ASTNode *expression = parse_expression(parser);
  if (expression == NULL) {
    throw_parser_error("Error: expected expression found NULL");
  }

  stmt->as.let.identifier = expression->as.assign.identifier;
  stmt->as.let.value = expression->as.assign.value;

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

// ifStmt -> "if" "(" equality ")" block ("else" block)?
ASTNode *parse_if_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_IF)
    return NULL;
  advance(parser);
  // expect (
  expect(parser, VALUE_LEFT_PAREN);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_IF;

  ASTNode *eq = parse_equality(parser);
  if (eq == NULL)
    throw_parser_error("Error: expected equality in if statement");
  stmt->as.if_stmt.condition = eq;

  // expect )
  expect(parser, VALUE_RIGHT_PAREN);

  stmt->as.if_stmt.then_branch = parse_block(parser);

  // see if we have else
  if (peek(parser)->type == VALUE_ELSE) {
    advance(parser);
    stmt->as.if_stmt.else_branch = parse_block(parser);
  }
  return stmt;
}

// whileStmt -> "while" "(" equality ")" block
ASTNode *parse_while_stmt(Parser *parser) {
  if (peek(parser)->type != VALUE_WHILE)
    return NULL;
  advance(parser);
  expect(parser, VALUE_LEFT_PAREN);

  ASTNode *stmt = arena_alloc(parser->arena, sizeof(*stmt));
  stmt->type = NODE_WHILE;

  ASTNode *eq = parse_equality(parser);
  if (eq == NULL)
    throw_parser_error("Error: expected equality in while statement");
  stmt->as.while_stmt.condition = eq;

  // expect )
  expect(parser, VALUE_RIGHT_PAREN);
  stmt->as.while_stmt.body = parse_block(parser);

  return stmt;
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
    Token *identifier = advance(parser);
    if (peek(parser)->type != VALUE_ASSIGN) {
      parser->current--;
      return parse_equality(parser);
    }
    ASTNode *assgn = arena_alloc(parser->arena, sizeof(*assgn));

    assgn->type = NODE_ASSIGN;
    assgn->as.assign.identifier = arena_alloc(parser->arena, sizeof(ASTNode));
    assgn->as.assign.identifier->type = NODE_IDENTIFIER;

    char *s = identifier->value.s;
    size_t len = strlen(s);
    assgn->as.assign.identifier->as.identifier.value =
        arena_alloc(parser->arena, len + 1);
    memcpy(assgn->as.assign.identifier->as.identifier.value, s, len);
    assgn->as.assign.identifier->as.identifier.value[len] = '\0';

    expect(parser, VALUE_ASSIGN);
    ASTNode *value = parse_assignment(parser);
    assgn->as.assign.value = value;

    return assgn;
  }
  return parse_equality(parser);
}

// equality -> comparison (("==") comparison)?
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

  ASTNode *term = NULL;

  switch (peek(parser)->type) {
  case VALUE_PLUS:
    term = arena_alloc(parser->arena, sizeof(*term));
    term->type = NODE_BINARY;
    term->as.binary.op = VALUE_PLUS;
    break;
  case VALUE_MINUS:
    term = arena_alloc(parser->arena, sizeof(*term));
    term->type = NODE_BINARY;
    term->as.binary.op = VALUE_MINUS;
    break;
  default:
    break;
  }
  if (term != NULL) {
    advance(parser);
    ASTNode *right = parse_factor(parser);
    if (right == NULL)
      throw_parser_error("Error: right factor needed for term");
    term->as.binary.left = left;
    term->as.binary.right = right;
    return term;
  }
  return left;
}

// factor -> unary (("*" | "/") unary)*
ASTNode *parse_factor(Parser *parser) {
  ASTNode *left = parse_unary(parser);
  if (left == NULL)
    return NULL;

  ASTNode *factor = NULL;

  switch (peek(parser)->type) {
  case VALUE_ASTERIK:
    factor = arena_alloc(parser->arena, sizeof(*factor));
    factor->type = NODE_BINARY;
    factor->as.binary.op = VALUE_ASTERIK;
    break;
  case VALUE_FORWARD_SLASH:
    factor = arena_alloc(parser->arena, sizeof(*factor));
    factor->type = NODE_BINARY;
    factor->as.binary.op = VALUE_FORWARD_SLASH;
    break;
  default:
    break;
  }
  if (factor != NULL) {
    advance(parser);
    ASTNode *right = parse_unary(parser);
    if (right == NULL)
      throw_parser_error("Error: right unary needed for factor");
    factor->as.binary.left = left;
    factor->as.binary.right = right;
    return factor;
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
  case NODE_ASSIGN:
    return "NODE_ASSIGN";
  case NODE_LET:
    return "NODE_LET";
  case NODE_BLOCK:
    return "NODE_BLOCK";
  case NODE_PRINT:
    return "NODE_PRINT";
  case NODE_IF:
    return "NODE_IF";
  case NODE_WHILE:
    return "NODE_WHILE";
  default:
    return "INVALID_NODE_TYPE";
  }
}
