#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"
static char *ler(const char*path,size_t*len){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<0){fclose(f);return NULL;}rewind(f);char*b=malloc((size_t)n+1);if(!b){fclose(f);return NULL;}size_t count=fread(b,1,(size_t)n,f);fclose(f);if(count!=(size_t)n){free(b);return NULL;}b[count]=0;if(count>=3&&(unsigned char)b[0]==0xef&&(unsigned char)b[1]==0xbb&&(unsigned char)b[2]==0xbf){memmove(b,b+3,count-3);count-=3;b[count]=0;}*len=count;return b;}
int main(int argc,char**argv){if(argc>2){fprintf(stderr,"Uso: %s [arquivo.java]\n",argv[0]);return 1;}const char*path=argc==2?argv[1]:"HelloWorld.java";size_t n=0;char*source=ler(path,&n);if(!source){fprintf(stderr,"Nao foi possivel ler %s\n",path);return 1;}
TabelaSimbolos sym;tabela_inicializar(&sym);ListaErros erros;lista_erros_inicializar(&erros);Lexer lexer;lexer_inicializar(&lexer,source,n,&sym,&erros);VetorTokens tokens;vetor_tokens_inicializar(&tokens);
for(;;){Token t=lexer_proximo_token(&lexer);vetor_tokens_adicionar(&tokens,t);if(t.tipo==TOK_FIM_ARQUIVO)break;}
printf("ANALISE SINTATICA - JAVA (SUBCONJUNTO)\nArquivo: %s\nTokens: %d | Identificadores unicos: %d\n",path,tokens.total-1,sym.total);
if(erros.total){printf("\nERROS LEXICOS (%d):\n",erros.total);for(int i=0;i<erros.total;i++)printf("Linha %d, coluna %d: %s\n",erros.itens[i].linha,erros.itens[i].coluna,erros.itens[i].mensagem);printf("Analise sintatica nao executada por existirem erros lexicos.\n");}else{ListaErrosSintaticos sintaticos;analisar_sintaxe(&tokens,&sintaticos);if(sintaticos.total){printf("\nERROS SINTATICOS (%d):\n",sintaticos.total);for(int i=0;i<sintaticos.total;i++)printf("Linha %d, coluna %d: %s\n",sintaticos.itens[i].linha,sintaticos.itens[i].coluna,sintaticos.itens[i].mensagem);}else printf("\nSintaxe valida no subconjunto implementado.\n");int rc=sintaticos.total?3:0;erros_sintaticos_liberar(&sintaticos);vetor_tokens_liberar(&tokens);tabela_liberar(&sym);lista_erros_liberar(&erros);free(source);return rc;}
vetor_tokens_liberar(&tokens);tabela_liberar(&sym);lista_erros_liberar(&erros);free(source);return 2;}
