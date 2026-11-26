#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

typedef struct {
    int line;
    int col;
    char msg[256];
} SemanticError;

typedef struct {
    SemanticError *items;
    int count;
    int cap;
} SemanticErrors;

void semantic_check(AST *root, SemanticErrors *errors);
void semantic_errors_free(SemanticErrors *errors);

#endif
