#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define TAMANHO_TABELA_HASH 211

typedef struct {
    char *nome;
    int linha;
    int coluna;
    int ocorrencias;
    int proximo;
} Simbolo;

typedef struct {
    Simbolo *itens;
    int total;
    int capacidade;
    int baldes[TAMANHO_TABELA_HASH];
} TabelaSimbolos;

void tabela_inicializar(TabelaSimbolos *tabela);
void tabela_inserir(TabelaSimbolos *tabela, const char *nome, int linha, int coluna);
void tabela_liberar(TabelaSimbolos *tabela);

#endif
