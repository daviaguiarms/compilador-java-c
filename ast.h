#ifndef AST_H
#define AST_H
#include "lexer.h"
typedef enum {N_PROGRAM,N_CLASS,N_METHOD,N_PARAM,N_BLOCK,N_VAR,N_IF,N_WHILE,N_FOR,N_RETURN,N_EXPR_STMT,N_ASSIGN,N_BINARY,N_UNARY,N_CALL,N_INDEX,N_MEMBER,N_IDENT,N_LITERAL,N_NEW,N_EMPTY} NodeKind;
typedef struct AST {NodeKind kind; char *text; char *type; int line,col; struct AST *child,*next;} AST;
AST *ast_new(NodeKind kind,const char *text,const char *type,int line,int col);
void ast_add(AST *parent,AST *child);
void ast_print(const AST *node,int level);
void ast_free(AST *node);
const char *ast_kind(NodeKind k);
#endif
