#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef struct {
    char *nome;
    int linha;
    int coluna;
    int ocorrencias;
} Simbolo;

typedef struct {
    Simbolo *itens;
    int total;
    int capacidade;
} TabelaSimbolos;

void tabela_inicializar(TabelaSimbolos *tabela);
void tabela_inserir(TabelaSimbolos *tabela, const char *nome, int linha, int coluna);
void tabela_liberar(TabelaSimbolos *tabela);

#endif
