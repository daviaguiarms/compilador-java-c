#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include "symbol_table.h"

typedef enum {
    TOK_FIM_ARQUIVO,
    TOK_IDENTIFICADOR,
    TOK_PALAVRA_CHAVE,
    TOK_LITERAL_INTEIRO,
    TOK_LITERAL_DECIMAL,
    TOK_LITERAL_TEXTO,
    TOK_LITERAL_CARACTERE,
    TOK_LITERAL_BOOLEANO,
    TOK_LITERAL_NULO,
    TOK_OPERADOR,
    TOK_DELIMITADOR,
    TOK_ERRO
} TipoToken;

typedef struct {
    TipoToken tipo;
    char *lexema;
    int linha;
    int coluna;
} Token;

typedef struct {
    Token *itens;
    int total;
    int capacidade;
} VetorTokens;

void vetor_tokens_inicializar(VetorTokens *vetor);
void vetor_tokens_adicionar(VetorTokens *vetor, Token token);
void vetor_tokens_liberar(VetorTokens *vetor);

#define TAMANHO_MENSAGEM_ERRO 256

typedef struct {
    int linha;
    int coluna;
    char mensagem[TAMANHO_MENSAGEM_ERRO];
} ErroLexico;

typedef struct {
    ErroLexico *itens;
    int total;
    int capacidade;
} ListaErros;

typedef struct {
    const char *fonte;
    size_t tamanho;
    size_t posicao;
    int linha;
    int coluna;

    TabelaSimbolos *simbolos;
    ListaErros *erros;

    int comentarios_linha;
    int comentarios_bloco;
} Lexer;

void lexer_inicializar(
    Lexer *lexer,
    const char *fonte,
    size_t tamanho,
    TabelaSimbolos *simbolos,
    ListaErros *erros
);

Token lexer_proximo_token(Lexer *lexer);

void lista_erros_inicializar(ListaErros *lista);
void lista_erros_liberar(ListaErros *lista);
void registrar_erro(ListaErros *lista, int linha, int coluna, const char *formato, ...);

const char *nome_tipo_token(TipoToken tipo);

void imprimir_lexema(const char *lexema);

#endif
