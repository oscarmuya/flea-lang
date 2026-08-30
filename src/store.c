#include "../include/store.h"
#include <stdlib.h>
#include <string.h>

// Thinking of linked list that we add to the front for new values
// This helps when we are freeing or accessing elements in a block
// of code they will appear at the front and accessed easily
// INFO: Might change later still a NOOb though
//
// Each node has block_id which is the scope they are valid for
// the block ids are incremental unique to every block of code including the
// global

// add it to front and makes it the head
void add_value(Node **head, char *name, int block_id, Value value) {
  Node *new_node = malloc(sizeof(*new_node));
  new_node->name = name;
  new_node->block_id = block_id;
  new_node->value = value;
  new_node->next = NULL;

  if (*head == NULL) {
    *head = new_node;
    return;
  }

  Node *temp_head = *head;
  new_node->next = temp_head;
  *head = new_node;
}

// remove first occurrence by that name
void remove_by_name(Node **head, char *name) {
  // if it is first
  if (strcmp((*head)->name, name) == 0) {
    (*head) = (*head)->next;
    return;
  }

  Node *current = *head;
  while (current->next != NULL && strcmp(current->next->name, name) != 0) {
    current = current->next;
  }

  if (current->next == NULL)
    return;

  Node *temp = current->next;
  current->next = current->next->next;
  free(temp);
}

// remove all block values
// this is our form of GC
//
// we will have a left and right pointer we move the right if that node needs
// removal we move right again until a node that doesnt need removal then we
// connect the left and right then we move left to right
void remove_by_block(Node **head, int block_id) {
  if (*head == NULL)
    return;

  // first strip off any matching nodes at the head, since left/right
  // walking can't unlink a node that comes before "left"
  while (*head != NULL && (*head)->block_id == block_id) {
    Node *temp = *head;
    *head = (*head)->next;
    free(temp);
  }

  if (*head == NULL)
    return;

  Node *left = *head;
  Node *right = left->next;

  while (right != NULL) {
    if (right->block_id == block_id) {
      // move right forward until we find a node that doesn't need removal
      while (right != NULL && right->block_id == block_id) {
        Node *temp = right;
        right = right->next;
        free(temp);
      }
      // connect left to the first surviving node
      left->next = right;
    } else {
      // both nodes are fine, move left up to right
      left = right;
      right = right->next;
    }
  }
}

// find first occurrence by that name
Node *find_by_name(Node *head, char *name) {
  Node *current = head;
  while (current != NULL) {
    if (strcmp(current->name, name) == 0)
      return current;
    current = current->next;
  }
  return NULL;
}
Node *find_by_name_and_block(Node *head, char *name, int block_id) {
  Node *current = head;
  while (current != NULL) {
    if (strcmp(current->name, name) == 0 && current->block_id == block_id)
      return current;
    current = current->next;
  }
  return NULL;
}
void *free_store(Node *head) {
  Node *current = head;
  while (current != NULL) {
    Node *temp = current;
    current = current->next;
    free(temp);
  }
  return NULL;
}
