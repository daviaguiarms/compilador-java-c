#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
typedef struct {Token *itens; int total; int capacidade;} VetorTokens;
typedef struct {int linha, coluna; char mensagem[256];} ErroSintatico;
typedef struct {ErroSintatico *itens; int total, capacidade;} ListaErrosSintaticos;
void vetor_tokens_inicializar(VetorTokens *v);
void vetor_tokens_adicionar(VetorTokens *v, Token t);
void vetor_tokens_liberar(VetorTokens *v);
void analisar_sintaxe(const VetorTokens *tokens, ListaErrosSintaticos *erros);
void erros_sintaticos_liberar(ListaErrosSintaticos *e);
#endif
