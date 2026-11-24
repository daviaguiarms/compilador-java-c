#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "symbol_table.h"

typedef struct {
    int tokens;
    int identificadores;
    int palavras_chave;
    int inteiros;
    int decimais;
    int textos;
    int caracteres;
    int booleanos;
    int nulos;
    int operadores;
    int delimitadores;
    int tokens_erro;
} Estatisticas;

static char *ler_arquivo(const char *caminho, size_t *tamanho_saida) {
    FILE *arquivo;
    long tamanho_longo;
    size_t tamanho;
    size_t lidos;
    char *conteudo;

    arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        return NULL;
    }

    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fclose(arquivo);
        return NULL;
    }

    tamanho_longo = ftell(arquivo);
    if (tamanho_longo < 0) {
        fclose(arquivo);
        return NULL;
    }
    rewind(arquivo);

    tamanho = (size_t)tamanho_longo;
    conteudo = (char *)malloc(tamanho + 1);
    if (conteudo == NULL) {
        fclose(arquivo);
        fprintf(stderr, "Erro fatal: memoria insuficiente.\n");
        return NULL;
    }

    lidos = fread(conteudo, 1, tamanho, arquivo);
    fclose(arquivo);

    if (lidos != tamanho) {
        free(conteudo);
        return NULL;
    }

    conteudo[lidos] = '\0';

    if (lidos >= 3 &&
        (unsigned char)conteudo[0] == 0xEF &&
        (unsigned char)conteudo[1] == 0xBB &&
        (unsigned char)conteudo[2] == 0xBF) {
        memmove(conteudo, conteudo + 3, lidos - 3);
        lidos -= 3;
        conteudo[lidos] = '\0';
    }

    *tamanho_saida = lidos;
    return conteudo;
}

static int contar_linhas(const char *conteudo, size_t tamanho) {
    int linhas = 1;
    size_t i = 0;

    while (i < tamanho) {
        if (conteudo[i] == '\r') {
            if (i + 1 < tamanho && conteudo[i + 1] == '\n') {
                if (i + 2 < tamanho) {
                    linhas++;
                }
                i += 2;
            } else {
                if (i + 1 < tamanho) {
                    linhas++;
                }
                i++;
            }
        } else if (conteudo[i] == '\n') {
            if (i + 1 < tamanho) {
                linhas++;
            }
            i++;
        } else {
            i++;
        }
    }

    return linhas;
}

static void imprimir_cabecalho(const char *caminho, size_t tamanho, int linhas) {
    printf("==============================================================================\n");
    printf(" ANALISADOR LEXICO - SUBLINGUAGEM JAVA\n");
    printf("==============================================================================\n");
    printf(" Arquivo : %s\n", caminho);
    printf(" Tamanho : %lu bytes\n", (unsigned long)tamanho);
    printf(" Linhas  : %d\n", linhas);
    printf("------------------------------------------------------------------------------\n");
}

static void imprimir_cabecalho_tokens(void) {
    printf("\n TOKENS RECONHECIDOS\n");
    printf("------------------------------------------------------------------------------\n");
    printf(" %5s  %5s  %6s  %-19s %s\n", "#", "Linha", "Coluna", "Tipo", "Lexema");
    printf("------------------------------------------------------------------------------\n");
}

static void contabilizar_token(Estatisticas *estatisticas, TipoToken tipo) {
    estatisticas->tokens++;

    switch (tipo) {
        case TOK_IDENTIFICADOR:     estatisticas->identificadores++; break;
        case TOK_PALAVRA_CHAVE:     estatisticas->palavras_chave++; break;
        case TOK_LITERAL_INTEIRO:   estatisticas->inteiros++; break;
        case TOK_LITERAL_DECIMAL:   estatisticas->decimais++; break;
        case TOK_LITERAL_TEXTO:     estatisticas->textos++; break;
        case TOK_LITERAL_CARACTERE: estatisticas->caracteres++; break;
        case TOK_LITERAL_BOOLEANO:  estatisticas->booleanos++; break;
        case TOK_LITERAL_NULO:      estatisticas->nulos++; break;
        case TOK_OPERADOR:          estatisticas->operadores++; break;
        case TOK_DELIMITADOR:       estatisticas->delimitadores++; break;
        case TOK_ERRO:              estatisticas->tokens_erro++; break;
        default: break;
    }
}

static void imprimir_tabela_simbolos(const TabelaSimbolos *tabela) {
    int i;
    int largura = 6;

    printf("\n TABELA DE SIMBOLOS INICIAL (identificadores)\n");
    printf("------------------------------------------------------------------------------\n");

    if (tabela->total == 0) {
        printf(" (nenhum identificador encontrado)\n");
        return;
    }

    for (i = 0; i < tabela->total; i++) {
        int comprimento = (int)strlen(tabela->itens[i].nome);
        if (comprimento > largura) {
            largura = comprimento;
        }
    }

    printf(" %5s  %-*s  %5s  %6s  %11s  %s\n",
           "#", largura, "Lexema", "Linha", "Coluna", "Ocorrencias", "Tipo");
    printf("------------------------------------------------------------------------------\n");

    for (i = 0; i < tabela->total; i++) {
        printf(" %5d  %-*s  %5d  %6d  %11d  %s\n",
               i + 1,
               largura,
               tabela->itens[i].nome,
               tabela->itens[i].linha,
               tabela->itens[i].coluna,
               tabela->itens[i].ocorrencias,
               "indefinido");
    }

    printf("------------------------------------------------------------------------------\n");
    printf(" %d identificador(es) unico(s); tipos serao definidos nas proximas fases.\n",
           tabela->total);
}

static void imprimir_erros(const ListaErros *erros) {
    int i;

    printf("\n ERROS LEXICOS\n");
    printf("------------------------------------------------------------------------------\n");

    if (erros->total == 0) {
        printf(" Nenhum erro lexico encontrado.\n");
        return;
    }

    for (i = 0; i < erros->total; i++) {
        printf(" [%d] Linha %d, Coluna %d: %s\n",
               i + 1,
               erros->itens[i].linha,
               erros->itens[i].coluna,
               erros->itens[i].mensagem);
    }
}

static void imprimir_resumo(
    const Estatisticas *estatisticas,
    const Lexer *lexer,
    const TabelaSimbolos *tabela,
    const ListaErros *erros
) {
    printf("\n RESUMO DA ANALISE\n");
    printf("------------------------------------------------------------------------------\n");
    printf(" %-32s %d\n", "Tokens reconhecidos", estatisticas->tokens);
    printf("   %-30s %d\n", "Identificadores", estatisticas->identificadores);
    printf("   %-30s %d\n", "Palavras-chave", estatisticas->palavras_chave);
    printf("   %-30s %d\n", "Literais inteiros", estatisticas->inteiros);
    printf("   %-30s %d\n", "Literais decimais", estatisticas->decimais);
    printf("   %-30s %d\n", "Literais de texto", estatisticas->textos);
    printf("   %-30s %d\n", "Literais de caractere", estatisticas->caracteres);
    printf("   %-30s %d\n", "Literais booleanos", estatisticas->booleanos);
    printf("   %-30s %d\n", "Literal nulo", estatisticas->nulos);
    printf("   %-30s %d\n", "Operadores", estatisticas->operadores);
    printf("   %-30s %d\n", "Delimitadores", estatisticas->delimitadores);
    printf("   %-30s %d\n", "Tokens de erro", estatisticas->tokens_erro);
    printf(" %-32s %d\n", "Simbolos unicos", tabela->total);
    printf(" %-32s %d\n", "Comentarios de linha", lexer->comentarios_linha);
    printf(" %-32s %d\n", "Comentarios de bloco", lexer->comentarios_bloco);
    printf(" %-32s %d\n", "Erros lexicos registrados", erros->total);
}

static void imprimir_uso(const char *programa) {
    printf("Uso: %s [arquivo.java]\n", programa);
    printf("\n");
    printf("  Se nenhum arquivo for informado, analisa HelloWorld.java.\n");
    printf("  -h, --help  mostra esta ajuda.\n");
}

int main(int argc, char **argv) {
    const char *caminho = "HelloWorld.java";
    char *fonte;
    size_t tamanho = 0;
    Lexer lexer;
    TabelaSimbolos simbolos;
    ListaErros erros;
    Estatisticas estatisticas;
    Token token;
    int indice = 0;
    int linhas;
    int codigo_saida;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        imprimir_uso(argv[0]);
        return 0;
    }

    if (argc > 2) {
        fprintf(stderr, "Erro: argumentos demais.\n");
        imprimir_uso(argv[0]);
        return 1;
    }

    if (argc == 2) {
        caminho = argv[1];
    }

    fonte = ler_arquivo(caminho, &tamanho);
    if (fonte == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir ou ler o arquivo '%s'.\n", caminho);
        return 1;
    }

    tabela_inicializar(&simbolos);
    lista_erros_inicializar(&erros);
    memset(&estatisticas, 0, sizeof(estatisticas));
    lexer_inicializar(&lexer, fonte, tamanho, &simbolos, &erros);

    linhas = contar_linhas(fonte, tamanho);
    imprimir_cabecalho(caminho, tamanho, linhas);
    imprimir_cabecalho_tokens();

    for (;;) {
        token = lexer_proximo_token(&lexer);

        if (token.tipo == TOK_FIM_ARQUIVO) {
            free(token.lexema);
            break;
        }

        indice++;
        printf(" %5d  %5d  %6d  %-19s ",
               indice, token.linha, token.coluna, nome_tipo_token(token.tipo));
        imprimir_lexema(token.lexema);
        printf("\n");

        contabilizar_token(&estatisticas, token.tipo);
        free(token.lexema);
    }

    if (indice == 0) {
        printf(" (nenhum token reconhecido)\n");
    }

    imprimir_tabela_simbolos(&simbolos);
    imprimir_erros(&erros);
    imprimir_resumo(&estatisticas, &lexer, &simbolos, &erros);

    printf("\n------------------------------------------------------------------------------\n");
    if (erros.total == 0) {
        printf(" Analise concluida sem erros lexicos.\n");
        codigo_saida = 0;
    } else {
        printf(" Analise concluida com %d erro(s) lexico(s).\n", erros.total);
        codigo_saida = 2;
    }

    tabela_liberar(&simbolos);
    lista_erros_liberar(&erros);
    free(fonte);

    return codigo_saida;
}
