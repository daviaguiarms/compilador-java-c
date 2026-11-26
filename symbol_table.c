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

static unsigned int calcular_hash(const char *nome) {
    unsigned int hash = 0;

    while (*nome != '\0') {
        hash = hash * 31 + (unsigned char)*nome;
        nome++;
    }
    return hash % TAMANHO_TABELA_HASH;
}

void tabela_inicializar(TabelaSimbolos *tabela) {
    int i;

    tabela->itens = NULL;
    tabela->total = 0;
    tabela->capacidade = 0;

    for (i = 0; i < TAMANHO_TABELA_HASH; i++) {
        tabela->baldes[i] = -1;
    }
}

static Simbolo *buscar_simbolo(TabelaSimbolos *tabela, const char *nome) {
    unsigned int indice = calcular_hash(nome);
    int atual = tabela->baldes[indice];

    while (atual != -1) {
        if (strcmp(tabela->itens[atual].nome, nome) == 0) {
            return &tabela->itens[atual];
        }
        atual = tabela->itens[atual].proximo;
    }
    return NULL;
}

void tabela_inserir(TabelaSimbolos *tabela, const char *nome, int linha, int coluna) {
    Simbolo *existente = buscar_simbolo(tabela, nome);
    unsigned int indice;
    int posicao;

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

    indice = calcular_hash(nome);
    posicao = tabela->total;

    tabela->itens[posicao].nome = duplicar_texto(nome);
    tabela->itens[posicao].linha = linha;
    tabela->itens[posicao].coluna = coluna;
    tabela->itens[posicao].ocorrencias = 1;
    tabela->itens[posicao].proximo = tabela->baldes[indice];

    tabela->baldes[indice] = posicao;
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
