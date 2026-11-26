#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    int linha;
    int coluna;
    char mensagem[TAMANHO_MENSAGEM_ERRO];
} ErroSintatico;

typedef struct {
    ErroSintatico *itens;
    int total;
    int capacidade;
} ListaErrosSintaticos;

void analisar_sintaxe(const VetorTokens *tokens, ListaErrosSintaticos *erros);
AST *construir_ast(const VetorTokens *tokens, ListaErrosSintaticos *erros);
void erros_sintaticos_liberar(ListaErrosSintaticos *erros);

#endif
