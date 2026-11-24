#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *dupstr(const char*s){if(!s)return NULL;size_t n=strlen(s)+1;char*p=malloc(n);if(!p)exit(3);memcpy(p,s,n);return p;}
AST*ast_new(NodeKind k,const char*text,const char*type,int line,int col){AST*n=calloc(1,sizeof(*n));if(!n)exit(3);n->kind=k;n->text=dupstr(text);n->type=dupstr(type);n->line=line;n->col=col;return n;}
void ast_add(AST*p,AST*c){if(!p||!c)return;if(!p->child)p->child=c;else{AST*t=p->child;while(t->next)t=t->next;t->next=c;}}
const char*ast_kind(NodeKind k){const char*names[]={"Programa","Classe","Metodo","Parametro","Bloco","Variavel","Condicional","Enquanto","Para","Retorno","Instrucao","Atribuicao","Operacao","Unario","Chamada","Indice","Membro","Identificador","Literal","Novo","Vazio"};return k>=0&&k<=N_EMPTY?names[k]:"Desconhecido";}
void ast_print(const AST*n,int level){for(;n;n=n->next){for(int i=0;i<level;i++)printf("  ");printf("%s",ast_kind(n->kind));if(n->text)printf(" [%s]",n->text);if(n->type)printf(" : %s",n->type);printf(" (linha %d)\n",n->line);ast_print(n->child,level+1);}}
void ast_free(AST*n){while(n){AST*next=n->next;ast_free(n->child);free(n->text);free(n->type);free(n);n=next;}}
