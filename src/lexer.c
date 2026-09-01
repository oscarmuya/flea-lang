#define _POSIX_C_SOURCE 200809L
#include "../include/lexer.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Lexical analysis
// Just create tokens of our code (pretty easy right!)

const int VARIABLE_LENGTH = 128;
const int COMMENT_LENGTH = 1024;

Token *tokenize(const char *source, int *token_count) {
  int capacity = 64;
  int count = 0;
  Token *tokens = malloc(sizeof(*tokens) * (size_t)capacity);

  if (tokens == NULL) {
    perror("tokens");
    return NULL;
  }

  while (source[count] != '\0') {
    if (*token_count >= capacity) {
      capacity *= 2;
      tokens = realloc(tokens, sizeof(*tokens) * (size_t)capacity);
    }

    // digit [0-9]
    if (isdigit((unsigned char)source[count])) {
      int32_t integer = 0;
      while (isdigit((unsigned char)source[count])) {
        integer = integer * 10 + (source[count] - '0');
        count++;
      }
      // check if there is a floating part
      double fl = 0.0;
      double divisor = 10;
      if (source[count] == '.') {
        count++;
        while (isdigit((unsigned char)source[count])) {
          fl = fl + (double)(source[count] - '0') / divisor;
          divisor *= 10.0;
          count++;
        }
        double f = (double)integer;
        tokens[*token_count].type = VALUE_FLOAT;
        tokens[*token_count].value.f = f + fl;
      } else {
        tokens[*token_count].type = VALUE_NUMBER;
        tokens[*token_count].value.i = integer;
      }
      *token_count += 1;
      continue;
    }
    // letter, underscore [a-z][A-Z]
    else if (isalpha((unsigned char)source[count]) || source[count] == '_') {
      char string[VARIABLE_LENGTH];
      int position = 0;
      while (isalpha((unsigned char)source[count]) || source[count] == '_' ||
             isdigit((unsigned char)source[count])) {
        if (position < VARIABLE_LENGTH - 1)
          string[position++] = source[count];
        count++;
      }

      string[position] = '\0';
      if (strcmp(string, "make") == 0) {
        tokens[(*token_count)++].type = VALUE_MAKE;
      } else if (strcmp(string, "be") == 0) {
        tokens[(*token_count)++].type = VALUE_BE;
      } else if (strcmp(string, "should") == 0) {
        tokens[(*token_count)++].type = VALUE_SHOULD;
      } else if (strcmp(string, "otherwise") == 0) {
        tokens[(*token_count)++].type = VALUE_THEN;
      } else if (strcmp(string, "whilst") == 0) {
        tokens[(*token_count)++].type = VALUE_WHILST;
      } else if (strcmp(string, "print") == 0) {
        tokens[(*token_count)++].type = VALUE_PRINT;
      } else if (strcmp(string, "true") == 0) {
        tokens[*token_count].type = VALUE_BOOL;
        tokens[*token_count].value.b = true;
        (*token_count)++;
      } else if (strcmp(string, "false") == 0) {
        tokens[*token_count].type = VALUE_BOOL;
        tokens[*token_count].value.b = false;
        (*token_count)++;
      } else {
        tokens[*token_count].type = VALUE_IDENTIFIER;
        tokens[*token_count].value.s = strdup(string);
        (*token_count)++;
      }
      continue;
    }
    // symbol + - * / ( ) { } ;
    else if (source[count] == '/') {
      // lets check if this is a comment , must start with // and end with \n
      if (source[count + 1] != '\0' && source[count + 1] == '/') {
        // we consume everythig until new line
        char string[VARIABLE_LENGTH];
        int position = 0;
        count += 2;
        while (source[count] != '\n' && source[count] != '\0') {
          if (position < VARIABLE_LENGTH - 1)
            string[position++] = source[count];
          count++;
        }

        string[position] = '\0';
        tokens[*token_count].type = VALUE_COMMENT;
        tokens[*token_count].value.s = strdup(string);
        (*token_count)++;
      } else
        tokens[(*token_count)++].type = VALUE_FORWARD_SLASH;

    } else if (source[count] == '+')
      tokens[(*token_count)++].type = VALUE_PLUS;
    else if (source[count] == '-')
      tokens[(*token_count)++].type = VALUE_MINUS;
    else if (source[count] == '*')
      tokens[(*token_count)++].type = VALUE_ASTERIK;
    else if (source[count] == '(')
      tokens[(*token_count)++].type = VALUE_LEFT_PAREN;
    else if (source[count] == ')')
      tokens[(*token_count)++].type = VALUE_RIGHT_PAREN;
    else if (source[count] == '{')
      tokens[(*token_count)++].type = VALUE_LEFT_BRACE;
    else if (source[count] == '}')
      tokens[(*token_count)++].type = VALUE_RIGHT_BRACE;
    else if (source[count] == ';')
      tokens[(*token_count)++].type = VALUE_SEMICOLON;
    else if (source[count] == ',')
      tokens[(*token_count)++].type = VALUE_COMMA;
    else if (source[count] == '!') {
      // we check if next is also !=
      if (source[count + 1] != '\0' && source[count + 1] == '=') {
        tokens[(*token_count)++].type = VALUE_NOT_EQUALS;
        count++;
      } else
        tokens[(*token_count)++].type = VALUE_EXCLAMATION;
    } else if (source[count] == '<') {
      // if <=
      if (source[count + 1] != '\0' && source[count + 1] == '=') {
        tokens[(*token_count)++].type = VALUE_LESS_THAN_EQUALS;
        count++;
      } else {
        tokens[(*token_count)++].type = VALUE_LESS_THAN;
      }
    } else if (source[count] == '>') {
      if (source[count + 1] != '\0' && source[count + 1] == '=') {
        tokens[(*token_count)++].type = VALUE_GREATER_THAN_EQUALS;
        count++;
      } else {
        tokens[(*token_count)++].type = VALUE_GREATER_THAN;
      }
    }
    // Quote " '
    else if (source[count] == '\'') {
      // we consume everything till the pair quote
      char string[VARIABLE_LENGTH];
      int position = 0;
      count++;
      while (source[count] != '\'' && source[count] != '\0') {
        if (position < VARIABLE_LENGTH - 1)
          string[position++] = source[count];
        count++;
      }

      string[position] = '\0';
      tokens[*token_count].type = VALUE_STRING;
      tokens[*token_count].value.s = strdup(string);
      (*token_count)++;
    } else if (source[count] == '"') {
      // we consume everything till the pair quote
      char string[VARIABLE_LENGTH];
      int position = 0;
      count++;
      while (source[count] != '"' && source[count] != '\0') {
        if (position < VARIABLE_LENGTH - 1)
          string[position++] = source[count];
        count++;
      }

      string[position] = '\0';
      tokens[*token_count].type = VALUE_STRING;
      tokens[*token_count].value.s = strdup(string);
      (*token_count)++;
    }
    //  ==
    else if (source[count] == '=') {
      // we check if next is also =
      if (source[count + 1] != '\0' && source[count + 1] == '=') {
        tokens[(*token_count)++].type = VALUE_EQUALS;
        count++;
      }
    }
    // whitespace and new lines
    else if (source[count] == ' ' || source[count] == '\n') {
      // We do nothing
    }
    // unknown character
    else {
      tokens[*token_count].type = VALUE_UNKNOWN;
      tokens[*token_count].value.s = malloc(2);
      if (tokens[*token_count].value.s == NULL) {
        return NULL;
      }

      tokens[*token_count].value.s[0] = source[count];
      tokens[*token_count].value.s[1] = '\0';

      (*token_count)++;
    }

    count++;
  }

  return tokens;
}

void free_tokens(Token *tokens, int *token_count) {

  for (size_t i = 0; i < (size_t)*token_count; i++) {
    if (tokens[i].type == VALUE_IDENTIFIER || tokens[i].type == VALUE_UNKNOWN ||
        tokens[i].type == VALUE_STRING || tokens[i].type == VALUE_COMMENT) {
      free(tokens[i].value.s);
    }
  }

  free(tokens);
}

void visualize_tokens(Token *tokens, int *token_count) {
  for (size_t i = 0; i < (size_t)*token_count; i++) {

    switch (tokens[i].type) {
    case VALUE_MAKE:
      printf("MAKE\n");
      break;

    case VALUE_IDENTIFIER:
      printf("IDENTIFIER(%s)\n", tokens[i].value.s);
      break;

    case VALUE_NUMBER:
      printf("NUMBER(%d)\n", tokens[i].value.i);
      break;

    case VALUE_FLOAT:
      printf("FLOAT(%f)\n", tokens[i].value.f);
      break;

    case VALUE_STRING:
      printf("STRING(\"%s\")\n", tokens[i].value.s);
      break;

    case VALUE_BOOL:
      printf("BOOL(\"%d\")\n", tokens[i].value.b);
      break;

    case VALUE_BE:
      printf("BE: =\n");
      break;

    case VALUE_PLUS:
      printf("PLUS: +\n");
      break;

    case VALUE_MINUS:
      printf("MINUS: -\n");
      break;

    case VALUE_FORWARD_SLASH:
      printf("FORWARD_SLASH: /\n");
      break;

    case VALUE_ASTERIK:
      printf("ASTERIK: *\n");
      break;

    case VALUE_SHOULD:
      printf("SHOULD\n");
      break;

    case VALUE_THEN:
      printf("THEN\n");
      break;

    case VALUE_WHILST:
      printf("WHILST\n");
      break;

    case VALUE_LEFT_PAREN:
      printf("LEFT_PAREN: (\n");
      break;

    case VALUE_RIGHT_PAREN:
      printf("RIGHT_PAREN: )\n");
      break;

    case VALUE_LEFT_BRACE:
      printf("LEFT_BRACE: {\n");
      break;

    case VALUE_RIGHT_BRACE:
      printf("RIGHT_BRACE: }\n");
      break;

    case VALUE_EQUALS:
      printf("EQUALS: ==\n");
      break;

    case VALUE_NOT_EQUALS:
      printf("NOT EQUALS: !=\n");
      break;

    case VALUE_LESS_THAN:
      printf("LESS_THAN: <\n");
      break;

    case VALUE_GREATER_THAN:
      printf("GREATER_THAN: >\n");
      break;

    case VALUE_LESS_THAN_EQUALS:
      printf("LESS_THAN_EQUALS: <=\n");
      break;

    case VALUE_GREATER_THAN_EQUALS:
      printf("GREATER_THAN_EQUALS: >=\n");
      break;

    case VALUE_EXCLAMATION:
      printf("EXCLAMATION: !\n");
      break;

    case VALUE_PRINT:
      printf("PRINT\n");
      break;

    case VALUE_COMMENT:
      printf("COMMENT(%s)\n", tokens[i].value.s);
      break;

    case VALUE_COMMA:
      printf("COMMA: ,\n");
      break;

    case VALUE_SEMICOLON:
      printf("SEMICOLON: ;\n");
      break;

    case VALUE_UNKNOWN:
      printf("UNKNOWN(%s);\n", tokens[i].value.s);
      break;

    default:
      printf("UNKNOWN TOKEN: %u\n", tokens[i].type);
      break;
    }
  }
}

const char *token_type_to_string(TokenType type) {
  switch (type) {
  case VALUE_MAKE:
    return "VALUE_MAKE";
  case VALUE_IDENTIFIER:
    return "VALUE_IDENTIFIER";
  case VALUE_NUMBER:
    return "VALUE_NUMBER";
  case VALUE_FLOAT:
    return "VALUE_FLOAT";
  case VALUE_STRING:
    return "VALUE_STRING";
  case VALUE_BOOL:
    return "VALUE_BOOL";
  case VALUE_BE:
    return "VALUE_BE";
  case VALUE_PLUS:
    return "VALUE_PLUS";
  case VALUE_MINUS:
    return "VALUE_MINUS";
  case VALUE_FORWARD_SLASH:
    return "VALUE_FORWARD_SLASH";
  case VALUE_ASTERIK:
    return "VALUE_ASTERIK";
  case VALUE_SHOULD:
    return "VALUE_SHOULD";
  case VALUE_THEN:
    return "VALUE_THEN";
  case VALUE_WHILST:
    return "VALUE_WHILST";
  case VALUE_LEFT_PAREN:
    return "VALUE_LEFT_PAREN";
  case VALUE_RIGHT_PAREN:
    return "VALUE_RIGHT_PAREN";
  case VALUE_LEFT_BRACE:
    return "VALUE_LEFT_BRACE";
  case VALUE_RIGHT_BRACE:
    return "VALUE_RIGHT_BRACE";
  case VALUE_EQUALS:
    return "VALUE_EQUALS";
  case VALUE_NOT_EQUALS:
    return "VALUE_NOT_EQUALS";
  case VALUE_LESS_THAN:
    return "VALUE_LESS_THAN";
  case VALUE_LESS_THAN_EQUALS:
    return "VALUE_LESS_THAN_EQUALS";
  case VALUE_GREATER_THAN:
    return "VALUE_GREATER_THAN";
  case VALUE_GREATER_THAN_EQUALS:
    return "VALUE_GREATER_THAN_EQUALS";
  case VALUE_EXCLAMATION:
    return "VALUE_EXCLAMATION";
  case VALUE_PRINT:
    return "VALUE_PRINT";
  case VALUE_COMMENT:
    return "VALUE_COMMENT";
  case VALUE_COMMA:
    return "VALUE_COMMA";
  case VALUE_SEMICOLON:
    return "VALUE_SEMICOLON";
  case VALUE_UNKNOWN:
    return "VALUE_UNKNOWN";
  default:
    return "INVALID_TOKEN_TYPE";
  }
}
