#ifndef FLEA_LEXER_H
#define FLEA_LEXER_H

#include <stdint.h>
#include <stdbool.h>

extern const int VARIABLE_LENGTH;

typedef enum {
  VALUE_LET,
  VALUE_IDENTIFIER,

  VALUE_NUMBER,
  VALUE_STRING,
  VALUE_BOOL,

  VALUE_ASSIGN,
  VALUE_PLUS,
  VALUE_MINUS,
  VALUE_FORWARD_SLASH,
  VALUE_ASTERIK,

  VALUE_IF,
  VALUE_ELSE,
  VALUE_WHILE,

  VALUE_LEFT_PAREN,
  VALUE_RIGHT_PAREN,
  VALUE_LEFT_BRACE,
  VALUE_RIGHT_BRACE,

  VALUE_EQUALS,
  VALUE_NOT_EQUALS,
  VALUE_LESS_THAN,
  VALUE_LESS_THAN_EQUALS,
  VALUE_GREATER_THAN,
  VALUE_GREATER_THAN_EQUALS,
  VALUE_EXCLAMATION,

  VALUE_PRINT,
  VALUE_COMMENT,

  VALUE_SEMICOLON,
  VALUE_UNKNOWN,
} TokenType;

typedef struct Token Token;

struct Token {
  TokenType type;
  union {
    int32_t i;
    char *s;
    bool b;
  } value;
};

/*
 * Convert a stream of characters int an array of tokens
 * */
Token *tokenize(const char *source, int *token_count);
/*
 * Print out the tokens to visualize them
 * */
void visualize_tokens(Token *tokens, int *token_count);
/*
 * Free memory assigned to tokens
 * */
void free_tokens(Token *tokens, int *token_count);

const char *token_type_to_string(TokenType type);

#endif
