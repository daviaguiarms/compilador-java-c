#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *alocar(size_t tamanho) {
    void *ponteiro = malloc(tamanho);
    if (ponteiro == NULL) {
        fprintf(stderr, "Erro fatal: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    return ponteiro;
}

static char *duplicar_texto(const char *texto) {
    size_t tamanho;
    char *copia;

    if (texto == NULL) {
        return NULL;
    }

    tamanho = strlen(texto) + 1;
    copia = (char *)alocar(tamanho);
    memcpy(copia, texto, tamanho);
    return copia;
}

AST *ast_new(NodeKind kind, const char *text, const char *type, int line, int col) {
    AST *node = (AST *)calloc(1, sizeof(AST));

    if (node == NULL) {
        fprintf(stderr, "Erro fatal: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }

    node->kind = kind;
    node->text = duplicar_texto(text);
    node->type = duplicar_texto(type);
    node->line = line;
    node->col = col;
    return node;
}

void ast_add(AST *parent, AST *child) {
    AST *last;

    if (parent == NULL || child == NULL) {
        return;
    }

    if (parent->child == NULL) {
        parent->child = child;
        return;
    }

    last = parent->child;
    while (last->next != NULL) {
        last = last->next;
    }
    last->next = child;
}

const char *ast_kind(NodeKind kind) {
    static const char *names[] = {
        "Programa",
        "Classe",
        "Metodo",
        "Parametro",
        "Bloco",
        "Variavel",
        "Condicional",
        "Enquanto",
        "Para",
        "Retorno",
        "Instrucao",
        "Atribuicao",
        "Operacao",
        "Unario",
        "Chamada",
        "Indice",
        "Membro",
        "Identificador",
        "Literal",
        "Novo",
        "Vazio"
    };

    if (kind < 0 || kind > N_EMPTY) {
        return "Desconhecido";
    }
    return names[kind];
}

void ast_print(const AST *node, int level) {
    int i;

    if (node != NULL && level >= 128) {
        puts("... (limite de exibicao da AST)");
        return;
    }

    for (; node != NULL; node = node->next) {
        for (i = 0; i < level; i++) {
            printf("  ");
        }

        printf("%s", ast_kind(node->kind));
        if (node->text != NULL) {
            printf(" [%s]", node->text);
        }
        if (node->type != NULL) {
            printf(" : %s", node->type);
        }
        printf(" (linha %d)\n", node->line);

        ast_print(node->child, level + 1);
    }
}

void ast_free(AST *node) {
    while (node != NULL) {
        AST *next = node->next;
        /* Junta os filhos a lista para liberar sem recursao. */
        if (node->child != NULL) {
            AST *last = node->child;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = next;
            next = node->child;
        }
        free(node->text);
        free(node->type);
        free(node->inferred_type);
        free(node);
        node = next;
    }
}
