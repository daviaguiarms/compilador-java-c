#ifndef SEMANTIC_H
#define SEMANTIC_H
#include "parser.h"
#include "ast.h"
typedef struct {int line,col;char msg[256];} SemanticError;
typedef struct {SemanticError *items;int count,cap;} SemanticErrors;
AST *build_ast(const VetorTokens *tokens,SemanticErrors *errors);
void semantic_check(const AST *root,SemanticErrors *errors);
void semantic_errors_free(SemanticErrors *errors);
#endif
