#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *alocar(size_t tamanho) {
    void *ponteiro = malloc(tamanho ? tamanho : 1);
    if (ponteiro == NULL) {
        fprintf(stderr, "Erro fatal: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    return ponteiro;
}

static void *realocar(void *ponteiro, size_t tamanho) {
    void *novo = realloc(ponteiro, tamanho ? tamanho : 1);
    if (novo == NULL) {
        fprintf(stderr, "Erro fatal: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    return novo;
}

static char *duplicar_texto(const char *texto) {
    size_t tamanho = strlen(texto);
    char *copia = (char *)alocar(tamanho + 1);
    memcpy(copia, texto, tamanho + 1);
    return copia;
}

void tabela_inicializar(TabelaSimbolos *tabela) {
    tabela->itens = NULL;
    tabela->total = 0;
    tabela->capacidade = 0;
}

static Simbolo *buscar_simbolo(TabelaSimbolos *tabela, const char *nome) {
    int i;
    for (i = 0; i < tabela->total; i++) {
        if (strcmp(tabela->itens[i].nome, nome) == 0) {
            return &tabela->itens[i];
        }
    }
    return NULL;
}

void tabela_inserir(TabelaSimbolos *tabela, const char *nome, int linha, int coluna) {
    Simbolo *existente = buscar_simbolo(tabela, nome);

    if (existente != NULL) {
        existente->ocorrencias++;
        return;
    }

    if (tabela->total == tabela->capacidade) {
        int nova_capacidade = (tabela->capacidade == 0) ? 8 : tabela->capacidade * 2;
        Simbolo *novos_itens = (Simbolo *)realocar(
            tabela->itens,
            (size_t)nova_capacidade * sizeof(Simbolo)
        );
        tabela->itens = novos_itens;
        tabela->capacidade = nova_capacidade;
    }

    tabela->itens[tabela->total].nome = duplicar_texto(nome);
    tabela->itens[tabela->total].linha = linha;
    tabela->itens[tabela->total].coluna = coluna;
    tabela->itens[tabela->total].ocorrencias = 1;
    tabela->total++;
}

void tabela_liberar(TabelaSimbolos *tabela) {
    int i;
    for (i = 0; i < tabela->total; i++) {
        free(tabela->itens[i].nome);
    }
    free(tabela->itens);
    tabela->itens = NULL;
    tabela->total = 0;
    tabela->capacidade = 0;
}
