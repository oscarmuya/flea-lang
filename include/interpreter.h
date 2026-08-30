#ifndef FLEA_INTERPRETER_H
#define FLEA_INTERPRETER_H

#include "parser.h"
#include "store.h"
#include <stdbool.h>
#include <stdint.h>

Value eval(ASTNode *node, Environment *env, int block_id);

Value eval_expr(ASTNode *node, Environment *env, int block_id);
Value eval_value(ASTNode *node, Environment *env);

void execute_stmt(ASTNode *node, Environment *env, int block_id);

Value operate_on_integers(Value *left, Value *right, TokenType op);
Value operate_on_strings(Value *left, Value *right, TokenType op,
                         Environment *env);
Value operate_on_bools(Value *left, Value *right, TokenType op);

Value *get_value(Environment *env, char *name);
Value *get_value_with_block_id(Environment *env, char *name, int block_id);
Value *update_value(Environment *env, char *name, Value value, int block_id);
Value *create_value(Environment *env, char *name, Value value, int block_id);
void delete_value(Environment *env, char *name);

int get_block_id(Environment *env);

#endif
