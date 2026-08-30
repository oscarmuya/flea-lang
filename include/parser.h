#ifndef FLEA_PARSER_H
#define FLEA_PARSER_H

#include "arena.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  NODE_PROGRAM,

  NODE_NUMBER,
  NODE_STRING,
  NODE_BOOLEAN,
  NODE_IDENTIFIER,

  NODE_UNARY,
  NODE_BINARY,

  NODE_ASSIGN,
  NODE_LET,
  NODE_BLOCK,
  NODE_PRINT,
  NODE_COMMENT,

  NODE_IF,
  NODE_WHILE,
} NodeType;

typedef struct ASTNode ASTNode;

typedef struct {
  ASTNode **statements;
  int count;
} ProgramNode;

typedef struct {
  int32_t value;
} NumberNode;

typedef struct {
  char *value;
} StringNode;

typedef struct {
  bool value;
} BooleanNode;

typedef struct {
  char *value;
} IdentifierNode;

typedef struct {
  ASTNode *operand;
  TokenType op; // ! , -
} UnaryNode;

typedef struct {
  ASTNode *left;
  ASTNode *right;
  TokenType op; // + - == < > <= >=
} BinaryNode;

typedef struct {
  ASTNode *identifier;
  ASTNode *value;
} AssignNode;

typedef struct {
  ASTNode *identifier;
  ASTNode *value;
} LetNode;

typedef struct {
  ASTNode **statements;
  int count;
} BlockNode;

typedef struct {
  ASTNode *value;
} PrintNode;

typedef struct {
  ASTNode *value;
} CommentNode;

typedef struct {
  ASTNode *condition;
  ASTNode *then_branch;
  ASTNode *else_branch;
} IfNode;

typedef struct {
  ASTNode *condition;
  ASTNode *body;
} WhileNode;

struct ASTNode {
  NodeType type;
  union {
    ProgramNode program;

    NumberNode number;
    StringNode string;
    BooleanNode boolean;
    IdentifierNode identifier;

    UnaryNode unary;
    BinaryNode binary;

    AssignNode assign;
    LetNode let;
    BlockNode block;
    PrintNode print;
    CommentNode comment;

    IfNode if_stmt;
    WhileNode while_stmt;
  } as;
};

typedef struct {
  Token *tokens;
  int current;
  int total;
  Arena *arena;
} Parser;

/*
 * A function to consume the current token and move to the next.
 **/
Token *advance(Parser *parser);
/*
 * A function that checks the current token is what you expect,
 * consumes it if so, and errors if not (needed for things like the closing ")"
 * after an if-condition).
 **/
Token *expect(Parser *parsr, TokenType expected);

/*
 * A function to look at the current token without consuming it.
 **/
Token *peek(Parser *parser);
bool is_at_end(Parser *parser);

// program -> statement* EOF
ASTNode *parse_program(Parser *parser);

// Dispatches to the correct statement parser based on the current token
// statement   -> printStmt | exprStmt | letStmt | ifStmt | whileStmt | block
ASTNode *parse_statement(Parser *parser);

// printStmt -> "print" equality ";"
ASTNode *parse_print_stmt(Parser *parser);

// // [anythign]
ASTNode *parse_comment(Parser *parser);

// letStmt -> "let" expression ";"
ASTNode *parse_let_stmt(Parser *parser);

// exprStmt -> expression ";"
ASTNode *parse_expr_stmt(Parser *parser);

// ifStmt -> "if" "(" equality ")" block ("else" block)?
ASTNode *parse_if_stmt(Parser *parser);

// whileStmt -> "while" "(" equality ")" block
ASTNode *parse_while_stmt(Parser *parser);

// block -> "{" statement* "}"
ASTNode *parse_block(Parser *parser);

// expression -> assignment
ASTNode *parse_expression(Parser *parser);

// assignment -> IDENTIFIER "=" assignment | equality
ASTNode *parse_assignment(Parser *parser);

// equality -> comparison (("==") comparison)?
ASTNode *parse_equality(Parser *parser);

// comparison -> term (("<" | ">" | "<=" | ">=") term)?
ASTNode *parse_comparison(Parser *parser);

// term -> factor (("+" | "-") factor)*
ASTNode *parse_term(Parser *parser);

// factor -> unary (("*" | "/") unary)*
ASTNode *parse_factor(Parser *parser);

// unary -> ("!" | "-") unary | primary
ASTNode *parse_unary(Parser *parser);

// primary ::= NUMBER | STRING | IDENTIFIER | "(" expression ")"
ASTNode *parse_primary(Parser *parser);

#endif
