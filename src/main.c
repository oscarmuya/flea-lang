#include "../include/arena.h"
#include "../include/interpreter.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/store.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Check if the user passed additional arguments
  if (argc < 2) {
    printf("Please provide a .fl file\n");
    return 0;
  }

  char *file_name = argv[1];
  FILE *file;

  file = fopen(file_name, "r");
  if (file == NULL) {
    printf("File specified not found\n");
    return 0;
  }

  // Check the current size of file
  size_t file_size = 0;
  fseek(file, 0, SEEK_END);
  file_size = (size_t)ftell(file);

  if (file_size == 0) {
    printf("File specified is empty\n");
    return 0;
  }

  char *file_data = malloc(file_size + 1);

  fseek(file, 0, SEEK_SET);
  fread(file_data, file_size + 1, 1, file);

  file_data[file_size] = '\0';

  // lexical analysis
  int token_count = 0;
  Token *tokens = tokenize(file_data, &token_count);
  // visualize_tokens(tokens, &token_count);
  Arena arena = arena_init(1024 * 1024);
  // parsing
  Parser parser = (Parser){
      .tokens = tokens, .current = 0, .total = token_count, .arena = &arena};
  ASTNode *head = parse_program(&parser);
  // tree walking
  Node *store = NULL;
  Environment env =
      (Environment){.next_id = 0, .store = store, .arena = &arena};
  // TODO: capture return Value and do something
  eval(head, &env, 1);

  free_tokens(tokens, &token_count);
  free_store(env.store);
  arena_free(&arena);
  free(file_data);
  fclose(file);

  return 0;
}
