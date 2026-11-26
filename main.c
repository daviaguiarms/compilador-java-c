#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"

static char *ler_arquivo(const char *caminho, size_t *tamanho) {
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        return NULL;
    }

    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fclose(arquivo);
        return NULL;
    }

    long tamanho_arquivo = ftell(arquivo);
    if (tamanho_arquivo < 0) {
        fclose(arquivo);
        return NULL;
    }
    rewind(arquivo);

    char *fonte = malloc((size_t)tamanho_arquivo + 1);
    if (fonte == NULL) {
        fclose(arquivo);
        return NULL;
    }

    size_t lidos = fread(fonte, 1, (size_t)tamanho_arquivo, arquivo);
    fclose(arquivo);
    if (lidos != (size_t)tamanho_arquivo) {
        free(fonte);
        return NULL;
    }


    if (lidos >= 3 && memcmp(fonte, "\xEF\xBB\xBF", 3) == 0) {
        memmove(fonte, fonte + 3, lidos - 3);
        lidos -= 3;
    }
    fonte[lidos] = '\0';
    *tamanho = lidos;
    return fonte;
}

static void coletar_tokens(Lexer *lexer, VetorTokens *tokens) {
    Token token;
    do {
        token = lexer_proximo_token(lexer);
        vetor_tokens_adicionar(tokens, token);
    } while (token.tipo != TOK_FIM_ARQUIVO);
}

static void imprimir_tokens(const VetorTokens *tokens) {
    printf("\n=== TOKENS ===\n");
    for (int i = 0; i < tokens->total - 1; i++) {
        const Token *token = &tokens->itens[i];
        printf("%4d:%-3d %-23s ", token->linha, token->coluna, nome_tipo_token(token->tipo));
        imprimir_lexema(token->lexema);
        putchar('\n');
    }
}

static void imprimir_simbolos(const TabelaSimbolos *simbolos) {
    printf("\n=== TABELA DE SIMBOLOS LEXICA ===\n");
    for (int i = 0; i < simbolos->total; i++) {
        const Simbolo *simbolo = &simbolos->itens[i];
        printf("%-25s primeira ocorrencia %d:%d, total %d\n", simbolo->nome,
               simbolo->linha, simbolo->coluna, simbolo->ocorrencias);
    }
}

static int analisar_semantica(const VetorTokens *tokens) {
    ListaErrosSintaticos erros_ast = {0};
    SemanticErrors erros = {0};
    AST *raiz = construir_ast(tokens, &erros_ast);

    if (erros_ast.total == 0) {
        semantic_check(raiz, &erros);
    }

    printf("\n=== AST ===\n");
    ast_print(raiz, 0);
    printf("\n=== ERROS SEMANTICOS ===\n");
    for (int i = 0; i < erros_ast.total; i++) {
        const ErroSintatico *erro = &erros_ast.itens[i];
        printf("Linha %d, coluna %d: %s\n", erro->linha, erro->coluna, erro->mensagem);
    }
    for (int i = 0; i < erros.count; i++) {
        const SemanticError *erro = &erros.items[i];
        printf("Linha %d, coluna %d: %s\n", erro->line, erro->col, erro->msg);
    }

    int resultado = erros_ast.total || erros.count ? 4 : 0;
    if (resultado == 0) {
        puts("Nenhum.");
    }

    ast_free(raiz);
    erros_sintaticos_liberar(&erros_ast);
    semantic_errors_free(&erros);
    return resultado;
}

static int analisar_programa(const VetorTokens *tokens, const ListaErros *erros_lexicos) {
    printf("\n=== ERROS LEXICOS ===\n");
    if (erros_lexicos->total > 0) {
        for (int i = 0; i < erros_lexicos->total; i++) {
            const ErroLexico *erro = &erros_lexicos->itens[i];
            printf("Linha %d, coluna %d: %s\n", erro->linha, erro->coluna, erro->mensagem);
        }
        return 2;
    }
    puts("Nenhum.");

    ListaErrosSintaticos erros;
    analisar_sintaxe(tokens, &erros);
    printf("\n=== ANALISE SINTATICA ===\n");
    for (int i = 0; i < erros.total; i++) {
        const ErroSintatico *erro = &erros.itens[i];
        printf("Linha %d, coluna %d: %s\n", erro->linha, erro->coluna, erro->mensagem);
    }
    int houve_erro = erros.total > 0;
    erros_sintaticos_liberar(&erros);
    if (houve_erro) {
        return 3;
    }

    puts("Sintaxe aceita pela gramatica implementada.");
    return analisar_semantica(tokens);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s arquivo.java\n", argv[0]);
        return 1;
    }

    size_t tamanho = 0;
    char *fonte = ler_arquivo(argv[1], &tamanho);
    if (fonte == NULL) {
        fprintf(stderr, "Nao foi possivel abrir: %s\n", argv[1]);
        return 1;
    }

    TabelaSimbolos simbolos;
    ListaErros erros_lexicos;
    VetorTokens tokens;
    Lexer lexer;
    tabela_inicializar(&simbolos);
    lista_erros_inicializar(&erros_lexicos);
    vetor_tokens_inicializar(&tokens);
    lexer_inicializar(&lexer, fonte, tamanho, &simbolos, &erros_lexicos);
    coletar_tokens(&lexer, &tokens);

    printf("COMPILADOR DIDATICO JAVA / C\n"
           "Grupo: Bruno Trindade Friaça Moreira; Davi Aguiar Macedo Santos; "
           "Gustavo Costa Nascimento; Lucas Henrique de Souza e Sá\n"
           "Arquivo: %s\n", argv[1]);
    imprimir_tokens(&tokens);
    imprimir_simbolos(&simbolos);
    int resultado = analisar_programa(&tokens, &erros_lexicos);

    const char *mensagens[] = {
        "APROVADO NAS TRES ANALISES", "ERRO DE ENTRADA",
        "REPROVADO: ERRO LEXICO", "REPROVADO: ERRO SINTATICO",
        "REPROVADO: ERRO SEMANTICO"
    };
    printf("\n=== RESULTADO ===\n%s\n", mensagens[resultado]);

    vetor_tokens_liberar(&tokens);
    tabela_liberar(&simbolos);
    lista_erros_liberar(&erros_lexicos);
    free(fonte);
    return resultado;
}
