#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"
static char *read_file(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long size=ftell(f);if(size<0){fclose(f);return NULL;}rewind(f);char*b=malloc((size_t)size+1);if(!b){fclose(f);return NULL;}size_t len=fread(b,1,(size_t)size,f);fclose(f);if(len!=(size_t)size){free(b);return NULL;}b[len]=0;if(len>=3&&(unsigned char)b[0]==239&&(unsigned char)b[1]==187&&(unsigned char)b[2]==191){memmove(b,b+3,len-3);len-=3;b[len]=0;}*n=len;return b;}
int main(int argc,char**argv){if(argc!=2){fprintf(stderr,"Uso: %s arquivo.java\n",argv[0]);return 1;}size_t len=0;char*source=read_file(argv[1],&len);if(!source){fprintf(stderr,"Nao foi possivel abrir: %s\n",argv[1]);return 1;}TabelaSimbolos syms;tabela_inicializar(&syms);ListaErros lex_errors;lista_erros_inicializar(&lex_errors);Lexer lexer;lexer_inicializar(&lexer,source,len,&syms,&lex_errors);VetorTokens v;vetor_tokens_inicializar(&v);
for(;;){Token t=lexer_proximo_token(&lexer);vetor_tokens_adicionar(&v,t);if(t.tipo==TOK_FIM_ARQUIVO)break;}
printf("COMPILADOR DIDATICO JAVA / C\nGrupo: Bruno Trindade Friaça Moreira; Davi Aguiar Macedo Santos; Gustavo Costa Nascimento; Lucas Henrique de Souza e Sá\nArquivo: %s\n\n=== TOKENS ===\n",argv[1]);for(int i=0;i<v.total-1;i++)printf("%4d:%-3d %-23s %s\n",v.itens[i].linha,v.itens[i].coluna,nome_tipo_token(v.itens[i].tipo),v.itens[i].lexema);
printf("\n=== TABELA DE SIMBOLOS LEXICA ===\n");for(int i=0;i<syms.total;i++)printf("%-25s primeira ocorrencia %d:%d, total %d\n",syms.itens[i].nome,syms.itens[i].linha,syms.itens[i].coluna,syms.itens[i].ocorrencias);
int rc=0;printf("\n=== ERROS LEXICOS ===\n");if(lex_errors.total){for(int i=0;i<lex_errors.total;i++)printf("Linha %d, coluna %d: %s\n",lex_errors.itens[i].linha,lex_errors.itens[i].coluna,lex_errors.itens[i].mensagem);rc=2;}else puts("Nenhum.");
if(!rc){ListaErrosSintaticos syn;analisar_sintaxe(&v,&syn);printf("\n=== ANALISE SINTATICA ===\n");if(syn.total){for(int i=0;i<syn.total;i++)printf("Linha %d, coluna %d: %s\n",syn.itens[i].linha,syn.itens[i].coluna,syn.itens[i].mensagem);rc=3;}else puts("Sintaxe aceita pela gramatica implementada.");
if(!rc){SemanticErrors errors={0};AST*root=build_ast(&v,&errors);if(!errors.count)semantic_check(root,&errors);printf("\n=== AST ===\n");ast_print(root,0);printf("\n=== ERROS SEMANTICOS ===\n");if(errors.count){for(int i=0;i<errors.count;i++)printf("Linha %d, coluna %d: %s\n",errors.items[i].line,errors.items[i].col,errors.items[i].msg);rc=4;}else puts("Nenhum.");ast_free(root);semantic_errors_free(&errors);}
erros_sintaticos_liberar(&syn);}printf("\n=== RESULTADO ===\n%s\n",rc==0?"APROVADO NAS TRES ANALISES":rc==2?"REPROVADO: ERRO LEXICO":rc==3?"REPROVADO: ERRO SINTATICO":"REPROVADO: ERRO SEMANTICO");
vetor_tokens_liberar(&v);tabela_liberar(&syms);lista_erros_liberar(&lex_errors);free(source);return rc;}
