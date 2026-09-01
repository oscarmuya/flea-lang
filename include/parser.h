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

  NODE_MAKE,
  NODE_BE,
  NODE_BLOCK,
  NODE_PRINT,
  NODE_COMMENT,

  NODE_SHOULD,
  NODE_WHILST,
  NODE_MAKE_FN,
  NODE_CALL_FN,
  NODE_PARAMS,
  NODE_ARGS
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
} BeNode;

typedef struct {
  ASTNode *identifier;
  ASTNode *value;
} MakeNode;

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
} ShouldNode;

typedef struct {
  ASTNode *condition;
  ASTNode *body;
} WhilstNode;

typedef struct {
  ASTNode *fn_name;
  ASTNode *params;
  ASTNode *body;
} MakeFnNode;

typedef struct {
  ASTNode *args;
  ASTNode *fn_name;
} CallFnNode;

typedef struct {
  ASTNode **params;
  size_t count;
} ParamsListNode;

typedef struct {
  ASTNode **args;
  size_t count;
} ArgsListNode;

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

    BeNode be;
    MakeNode make;
    BlockNode block;
    PrintNode print;
    CommentNode comment;

    ShouldNode should_stmt;
    WhilstNode whilst_stmt;

    MakeFnNode make_fn_stmt;
    CallFnNode call_fn_stmt;
    ParamsListNode params;
    ArgsListNode args;
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
Token *look_ahead(Parser *parser, size_t steps);
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

// makeStmt -> "make" expression ";"
ASTNode *parse_make_stmt(Parser *parser);

// exprStmt -> expression ";"
ASTNode *parse_expr_stmt(Parser *parser);

// shouldStmt -> "should" "(" equality ")" block ("then" block)?
ASTNode *parse_should_stmt(Parser *parser);

// whilstStmt -> "whilst" "(" equality ")" block
ASTNode *parse_whilst_stmt(Parser *parser);

// makeFnStmt  ::= "make" IDENTIFER "(" paramList ")" block
ASTNode *parse_make_fn_stmt(Parser *parser);

// callFnStmt  ::= IDENTIFIER "(" argList ")"
ASTNode *parse_call_fn_stmt(Parser *parser);

// paramList   ::= (IDENTIFER ("," IDENTIFER)*)?
ASTNode *parse_param_list(Parser *parser);

// argList     ::= (expression ("," expression)*)?
ASTNode *parse_arg_list(Parser *parser);

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
