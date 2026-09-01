#ifndef FLEA_STORE_H
#define FLEA_STORE_H

#include "arena.h"
#include "parser.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { VAL_NUMBER, VAL_STRING, VAL_BOOL, VAL_NIL, VAL_FN, VAL_FLOAT } ValueType;

typedef struct {
  ValueType type;
  union {
    int32_t integer;
    double flt;
    char *string;
    bool boolean;
    MakeFnNode *fn;
  } as;
} Value;

typedef struct node_t {
  Value value;
  char *name;
  struct node_t *next;
  int block_id;
} Node;

typedef struct {
  Node *store;
  int next_id;
  Arena *arena;
} Environment;

// add it to front and makes it the head
void add_value(Node **head, char *name, int block_id, Value value);

// remove first occurrence by that name
void remove_by_name(Node **head, char *name);

// remove all block values
void remove_by_block(Node **head, int block_id);

// find first occurrence by that name
Node *find_by_name(Node *head, char *name);

Node *find_by_name_and_block(Node *head, char *name, int block_id);

// free the store
void *free_store(Node *head);

#endif
