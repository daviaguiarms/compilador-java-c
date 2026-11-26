#include "parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const VetorTokens *tokens;
    int posicao;
    int profundidade;
    ListaErrosSintaticos *erros;
} Parser;

static const Token *token_relativo(Parser *parser, int deslocamento) {
    int indice = parser->posicao + deslocamento;

    if (indice < 0) {
        indice = 0;
    }
    if (indice >= parser->tokens->total) {
        indice = parser->tokens->total - 1;
    }
    return &parser->tokens->itens[indice];
}

static const Token *token_atual(Parser *parser) {
    return token_relativo(parser, 0);
}

static const char *lexema_relativo(Parser *parser, int deslocamento) {
    return token_relativo(parser, deslocamento)->lexema;
}

static const char *lexema_atual(Parser *parser) {
    return lexema_relativo(parser, 0);
}

static int esta_no_fim(Parser *parser) {
    return token_atual(parser)->tipo == TOK_FIM_ARQUIVO;
}

static int lexema_e(Parser *parser, const char *texto) {
    return strcmp(lexema_atual(parser), texto) == 0;
}

static void avancar(Parser *parser) {
    if (!esta_no_fim(parser)) {
        parser->posicao++;
    }
}

static int consumir(Parser *parser, const char *texto) {
    if (lexema_e(parser, texto)) {
        avancar(parser);
        return 1;
    }
    return 0;
}

static void registrar_erro_sintatico(Parser *parser, const char *formato, ...) {
    const Token *token = token_atual(parser);
    ListaErrosSintaticos *erros = parser->erros;
    ErroSintatico *erro;
    va_list argumentos;

    if (erros->total >= 100) {
        return;
    }

    if (erros->total == erros->capacidade) {
        int nova_capacidade = erros->capacidade ? erros->capacidade * 2 : 16;
        ErroSintatico *novos_itens = (ErroSintatico *)realloc(
            erros->itens,
            (size_t)nova_capacidade * sizeof(ErroSintatico)
        );
        if (novos_itens == NULL) {
            exit(3);
        }
        erros->itens = novos_itens;
        erros->capacidade = nova_capacidade;
    }

    erro = &erros->itens[erros->total];
    erro->linha = token->linha;
    erro->coluna = token->coluna;

    va_start(argumentos, formato);
    vsnprintf(erro->mensagem, sizeof(erro->mensagem), formato, argumentos);
    va_end(argumentos);

    erros->total++;
}

static int entrar_regra(Parser *parser) {
    if (parser->profundidade >= 128) {
        registrar_erro_sintatico(parser, "Limite de aninhamento sintatico excedido");
        parser->posicao = parser->tokens->total - 1;
        return 0;
    }
    parser->profundidade++;
    return 1;
}

static int exigir(Parser *parser, const char *texto) {
    if (consumir(parser, texto)) {
        return 1;
    }
    registrar_erro_sintatico(parser, "Esperado '%s', encontrado '%s'", texto, lexema_atual(parser));
    return 0;
}

static int e_identificador(Parser *parser) {
    return token_atual(parser)->tipo == TOK_IDENTIFICADOR ||
           lexema_e(parser, "var") ||
           lexema_e(parser, "record");
}

static int exigir_identificador(Parser *parser) {
    if (e_identificador(parser)) {
        avancar(parser);
        return 1;
    }
    registrar_erro_sintatico(parser, "Identificador esperado, encontrado '%s'", lexema_atual(parser));
    return 0;
}

static int inicia_tipo(Parser *parser) {
    const char *texto = lexema_atual(parser);

    return e_identificador(parser) ||
           strcmp(texto, "int") == 0 ||
           strcmp(texto, "long") == 0 ||
           strcmp(texto, "byte") == 0 ||
           strcmp(texto, "short") == 0 ||
           strcmp(texto, "boolean") == 0 ||
           strcmp(texto, "char") == 0 ||
           strcmp(texto, "double") == 0 ||
           strcmp(texto, "float") == 0 ||
           strcmp(texto, "void") == 0;
}

static int e_primitivo(Parser *parser) {
    const char *texto = lexema_atual(parser);

    return strcmp(texto, "int") == 0 ||
           strcmp(texto, "long") == 0 ||
           strcmp(texto, "byte") == 0 ||
           strcmp(texto, "short") == 0 ||
           strcmp(texto, "boolean") == 0 ||
           strcmp(texto, "char") == 0 ||
           strcmp(texto, "double") == 0 ||
           strcmp(texto, "float") == 0 ||
           strcmp(texto, "void") == 0;
}

static int e_modificador(Parser *parser) {
    const char *texto = lexema_atual(parser);

    return strcmp(texto, "public") == 0 ||
           strcmp(texto, "private") == 0 ||
           strcmp(texto, "protected") == 0 ||
           strcmp(texto, "static") == 0 ||
           strcmp(texto, "final") == 0 ||
           strcmp(texto, "abstract") == 0 ||
           strcmp(texto, "native") == 0 ||
           strcmp(texto, "synchronized") == 0 ||
           strcmp(texto, "strictfp") == 0 ||
           strcmp(texto, "transient") == 0 ||
           strcmp(texto, "volatile") == 0 ||
           strcmp(texto, "default") == 0;
}

static void parse_expression(Parser *parser, int precedencia_minima);
static void parse_statement(Parser *parser);
static void parse_type(Parser *parser);

static void parse_annotation(Parser *parser) {
    exigir(parser, "@");
    exigir_identificador(parser);

    while (consumir(parser, ".")) {
        exigir_identificador(parser);
    }

    if (consumir(parser, "(")) {
        if (!lexema_e(parser, ")")) {
            parse_expression(parser, 1);
            while (consumir(parser, ",")) {
                parse_expression(parser, 1);
            }
        }
        exigir(parser, ")");
    }
}

static void parse_modifiers(Parser *parser) {
    while (e_modificador(parser) || lexema_e(parser, "@")) {
        if (lexema_e(parser, "@")) {
            parse_annotation(parser);
        } else {
            avancar(parser);
        }
    }
}

static void parse_qualified_name(Parser *parser) {
    exigir_identificador(parser);

    while (consumir(parser, ".")) {
        exigir_identificador(parser);
    }
}

static void parse_generic(Parser *parser) {
    int profundidade;

    if (!consumir(parser, "<")) {
        return;
    }

    profundidade = 1;
    while (!esta_no_fim(parser) && profundidade > 0) {
        if (lexema_e(parser, "<")) {
            profundidade++;
            avancar(parser);
        } else if (lexema_e(parser, ">")) {
            profundidade--;
            avancar(parser);
        } else if (lexema_e(parser, ">>")) {
            profundidade -= 2;
            avancar(parser);
        } else if (lexema_e(parser, ">>>")) {
            profundidade -= 3;
            avancar(parser);
        } else {
            avancar(parser);
        }
    }

    if (profundidade != 0) {
        registrar_erro_sintatico(parser, "Lista de tipos genericos nao fechada corretamente");
    }
}

static void parse_type(Parser *parser) {
    int primitivo;

    if (!inicia_tipo(parser)) {
        registrar_erro_sintatico(parser, "Tipo esperado, encontrado '%s'", lexema_atual(parser));
        return;
    }

    primitivo = e_primitivo(parser);
    avancar(parser);

    if (!primitivo) {
        while (consumir(parser, ".")) {
            exigir_identificador(parser);
        }
        parse_generic(parser);
    }

    while (consumir(parser, "[")) {
        exigir(parser, "]");
    }
}

typedef struct {
    const char *operador;
    int valor;
} PrecedenciaOperador;

static const PrecedenciaOperador PRECEDENCIAS[] = {
    { "=", 1 },
    { "+=", 1 },
    { "-=", 1 },
    { "*=", 1 },
    { "/=", 1 },
    { "%=", 1 },
    { "&=", 1 },
    { "|=", 1 },
    { "^=", 1 },
    { "<<=", 1 },
    { ">>=", 1 },
    { ">>>=", 1 },
    { "?", 2 },
    { "||", 3 },
    { "&&", 4 },
    { "|", 5 },
    { "^", 6 },
    { "&", 7 },
    { "==", 8 },
    { "!=", 8 },
    { "<", 9 },
    { ">", 9 },
    { "<=", 9 },
    { ">=", 9 },
    { "instanceof", 9 },
    { "<<", 10 },
    { ">>", 10 },
    { ">>>", 10 },
    { "+", 11 },
    { "-", 11 },
    { "*", 12 },
    { "/", 12 },
    { "%", 12 }
};

static int precedencia(const char *operador) {
    size_t i;

    for (i = 0; i < sizeof(PRECEDENCIAS) / sizeof(PRECEDENCIAS[0]); i++) {
        if (strcmp(operador, PRECEDENCIAS[i].operador) == 0) {
            return PRECEDENCIAS[i].valor;
        }
    }
    return 0;
}

static int e_literal(Parser *parser) {
    TipoToken tipo = token_atual(parser)->tipo;

    return tipo == TOK_LITERAL_INTEIRO ||
           tipo == TOK_LITERAL_DECIMAL ||
           tipo == TOK_LITERAL_TEXTO ||
           tipo == TOK_LITERAL_CARACTERE ||
           tipo == TOK_LITERAL_BOOLEANO ||
           tipo == TOK_LITERAL_NULO;
}

static void parse_initializer(Parser *parser) {
    if (!entrar_regra(parser)) {
        return;
    }
    if (consumir(parser, "{")) {
        if (!lexema_e(parser, "}")) {
            parse_initializer(parser);
            while (consumir(parser, ",")) {
                if (lexema_e(parser, "}")) {
                    break;
                }
                parse_initializer(parser);
            }
        }
        exigir(parser, "}");
    } else {
        parse_expression(parser, 1);
    }
    parser->profundidade--;
}

static void parse_arguments(Parser *parser) {
    exigir(parser, "(");

    if (!lexema_e(parser, ")")) {
        parse_expression(parser, 1);
        while (consumir(parser, ",")) {
            parse_expression(parser, 1);
        }
    }

    exigir(parser, ")");
}

static int parece_lambda(Parser *parser) {
    int posicao_inicial = parser->posicao;
    int valido = 1;

    while (!esta_no_fim(parser) &&
           !lexema_e(parser, ")") &&
           parser->posicao - posicao_inicial < 24) {
        if (!e_identificador(parser) &&
            !inicia_tipo(parser) &&
            !lexema_e(parser, ",") &&
            !lexema_e(parser, "[") &&
            !lexema_e(parser, "]")) {
            valido = 0;
        }
        avancar(parser);
    }

    if (!lexema_e(parser, ")") || strcmp(lexema_relativo(parser, 1), "->") != 0) {
        valido = 0;
    }

    parser->posicao = posicao_inicial;
    return valido;
}

static void parse_primary(Parser *parser) {
    if (!entrar_regra(parser)) {
        return;
    }

    if (e_literal(parser) ||
        e_identificador(parser) ||
        lexema_e(parser, "this") ||
        lexema_e(parser, "super")) {
        avancar(parser);
    } else if (consumir(parser, "new")) {
        if (inicia_tipo(parser)) {
            int primitivo = e_primitivo(parser);
            avancar(parser);
            if (!primitivo) {
                while (consumir(parser, ".")) {
                    exigir_identificador(parser);
                }
                parse_generic(parser);
            }
        } else {
            registrar_erro_sintatico(parser, "Tipo esperado apos new");
        }

        if (lexema_e(parser, "(")) {
            parse_arguments(parser);
            if (lexema_e(parser, "{")) {
                int profundidade = 0;
                do {
                    if (lexema_e(parser, "{")) {
                        profundidade++;
                    } else if (lexema_e(parser, "}")) {
                        profundidade--;
                    }
                    avancar(parser);
                } while (!esta_no_fim(parser) && profundidade > 0);
            }
        } else if (consumir(parser, "[")) {
            if (!lexema_e(parser, "]")) {
                parse_expression(parser, 1);
            }
            exigir(parser, "]");

            while (consumir(parser, "[")) {
                if (!lexema_e(parser, "]")) {
                    parse_expression(parser, 1);
                }
                exigir(parser, "]");
            }

            if (lexema_e(parser, "{")) {
                parse_initializer(parser);
            }
        } else {
            registrar_erro_sintatico(parser, "Esperado construtor ou indice apos 'new'");
        }
    } else if (consumir(parser, "(")) {
        if (parece_lambda(parser)) {
            while (!lexema_e(parser, ")") && !esta_no_fim(parser)) {
                avancar(parser);
            }
            exigir(parser, ")");
            exigir(parser, "->");

            if (lexema_e(parser, "{")) {
                parse_statement(parser);
            } else {
                parse_expression(parser, 1);
            }
        } else {
            parse_expression(parser, 1);
            exigir(parser, ")");
        }
    } else if (consumir(parser, "{")) {
        if (!lexema_e(parser, "}")) {
            parse_initializer(parser);
            while (consumir(parser, ",")) {
                parse_initializer(parser);
            }
        }
        exigir(parser, "}");
    } else {
        registrar_erro_sintatico(parser, "Expressao esperada, encontrado '%s'", lexema_atual(parser));
        if (!esta_no_fim(parser)) {
            avancar(parser);
        }
    }

    while (!esta_no_fim(parser)) {
        if (lexema_e(parser, "(")) {
            parse_arguments(parser);
        } else if (consumir(parser, ".")) {
            if (lexema_e(parser, "class")) {
                avancar(parser);
            } else {
                exigir_identificador(parser);
            }
        } else if (consumir(parser, "[")) {
            parse_expression(parser, 1);
            exigir(parser, "]");
        } else if (consumir(parser, "::")) {
            if (lexema_e(parser, "new")) {
                avancar(parser);
            } else {
                exigir_identificador(parser);
            }
        } else if (consumir(parser, "++") || consumir(parser, "--")) {
            continue;
        } else if (consumir(parser, "->")) {
            if (lexema_e(parser, "{")) {
                parse_statement(parser);
            } else {
                parse_expression(parser, 1);
            }
        } else {
            break;
        }
    }

    parser->profundidade--;
}

static void parse_unary(Parser *parser) {
    if (!entrar_regra(parser)) {
        return;
    }
    if (lexema_e(parser, "!") ||
        lexema_e(parser, "~") ||
        lexema_e(parser, "+") ||
        lexema_e(parser, "-") ||
        lexema_e(parser, "++") ||
        lexema_e(parser, "--")) {
        avancar(parser);
        parse_unary(parser);
    } else {
        parse_primary(parser);
    }
    parser->profundidade--;
}

static void parse_expression(Parser *parser, int precedencia_minima) {
    if (!entrar_regra(parser)) {
        return;
    }
    int posicao_inicial = parser->posicao;

    parse_unary(parser);

    while (!esta_no_fim(parser)) {
        int precedencia_atual = precedencia(lexema_atual(parser));
        char operador[16];
        int associativo_direita;

        if (precedencia_atual < precedencia_minima) {
            break;
        }

        snprintf(operador, sizeof(operador), "%s", lexema_atual(parser));
        avancar(parser);

        if (strcmp(operador, "?") == 0) {
            parse_expression(parser, 1);
            exigir(parser, ":");
            parse_expression(parser, 2);
        } else if (strcmp(operador, "instanceof") == 0) {
            parse_type(parser);
        } else {
            associativo_direita = (precedencia_atual == 1);
            parse_expression(parser, precedencia_atual + (associativo_direita ? 0 : 1));
        }

        if (parser->posicao == posicao_inicial) {
            break;
        }
    }
    parser->profundidade--;
}

static void parse_var_tail(Parser *parser) {
    while (consumir(parser, "[")) {
        exigir(parser, "]");
    }

    if (consumir(parser, "=")) {
        parse_initializer(parser);
    }

    while (consumir(parser, ",")) {
        exigir_identificador(parser);
        while (consumir(parser, "[")) {
            exigir(parser, "]");
        }
        if (consumir(parser, "=")) {
            parse_initializer(parser);
        }
    }
}

static int parece_declaracao(Parser *parser) {
    int posicao_inicial = parser->posicao;
    int limite = 0;
    int ok;

    if (e_modificador(parser)) {
        return 1;
    }

    if (!inicia_tipo(parser)) {
        return 0;
    }

    if (e_primitivo(parser) || e_identificador(parser)) {
        avancar(parser);

        while (consumir(parser, ".")) {
            if (!e_identificador(parser)) {
                break;
            }
            avancar(parser);
        }

        if (lexema_e(parser, "<")) {
            int profundidade = 0;
            do {
                if (lexema_e(parser, "<")) {
                    profundidade++;
                }
                if (lexema_e(parser, ">")) {
                    profundidade--;
                }
                if (lexema_e(parser, ">>")) {
                    profundidade -= 2;
                }
                if (lexema_e(parser, ">>>")) {
                    profundidade -= 3;
                }
                avancar(parser);
            } while (!esta_no_fim(parser) && profundidade > 0 && ++limite < 80);
        }

        while (consumir(parser, "[")) {
            if (!consumir(parser, "]")) {
                break;
            }
        }
    }

    ok = e_identificador(parser);
    parser->posicao = posicao_inicial;
    return ok;
}

static void parse_var_declaration(Parser *parser, int termina_com_ponto_virgula) {
    parse_type(parser);
    exigir_identificador(parser);
    parse_var_tail(parser);

    if (termina_com_ponto_virgula) {
        exigir(parser, ";");
    }
}

static void parse_block(Parser *parser) {
    int contador = 0;

    exigir(parser, "{");

    while (!esta_no_fim(parser) &&
           !lexema_e(parser, "}") &&
           contador++ < 100000) {
        int posicao_inicial = parser->posicao;

        parse_statement(parser);

        if (parser->posicao == posicao_inicial) {
            registrar_erro_sintatico(parser, "Nao foi possivel avancar na instrucao");
            avancar(parser);
        }
    }

    exigir(parser, "}");
}

static void parse_statement_impl(Parser *parser);

static void parse_statement(Parser *parser) {
    if (!entrar_regra(parser)) {
        return;
    }
    parse_statement_impl(parser);
    parser->profundidade--;
}

static void parse_statement_impl(Parser *parser) {
    if (esta_no_fim(parser)) {
        return;
    }

    if (lexema_e(parser, "{")) {
        parse_block(parser);
        return;
    }

    if (consumir(parser, ";")) {
        return;
    }

    if (consumir(parser, "if")) {
        exigir(parser, "(");
        parse_expression(parser, 1);
        exigir(parser, ")");
        parse_statement(parser);
        if (consumir(parser, "else")) {
            parse_statement(parser);
        }
        return;
    }

    if (consumir(parser, "while") ||
        consumir(parser, "switch") ||
        consumir(parser, "synchronized")) {
        exigir(parser, "(");
        parse_expression(parser, 1);
        exigir(parser, ")");
        parse_statement(parser);
        return;
    }

    if (consumir(parser, "for")) {
        exigir(parser, "(");

        if (!lexema_e(parser, ";")) {
            if (parece_declaracao(parser)) {
                parse_var_declaration(parser, 0);
            } else {
                parse_expression(parser, 1);
                while (consumir(parser, ",")) {
                    parse_expression(parser, 1);
                }
            }
        }

        if (consumir(parser, ":")) {
            parse_expression(parser, 1);
            exigir(parser, ")");
            parse_statement(parser);
            return;
        }

        exigir(parser, ";");

        if (!lexema_e(parser, ";")) {
            parse_expression(parser, 1);
        }
        exigir(parser, ";");

        if (!lexema_e(parser, ")")) {
            parse_expression(parser, 1);
            while (consumir(parser, ",")) {
                parse_expression(parser, 1);
            }
        }
        exigir(parser, ")");

        parse_statement(parser);
        return;
    }

    if (consumir(parser, "do")) {
        parse_statement(parser);
        exigir(parser, "while");
        exigir(parser, "(");
        parse_expression(parser, 1);
        exigir(parser, ")");
        exigir(parser, ";");
        return;
    }

    if (consumir(parser, "return") || consumir(parser, "throw")) {
        if (!lexema_e(parser, ";")) {
            parse_expression(parser, 1);
        }
        exigir(parser, ";");
        return;
    }

    if (consumir(parser, "break") || consumir(parser, "continue")) {
        if (e_identificador(parser)) {
            avancar(parser);
        }
        exigir(parser, ";");
        return;
    }

    if (consumir(parser, "try")) {
        if (consumir(parser, "(")) {
            while (!esta_no_fim(parser) && !lexema_e(parser, ")")) {
                avancar(parser);
            }
            exigir(parser, ")");
        }

        parse_block(parser);

        while (consumir(parser, "catch")) {
            exigir(parser, "(");
            parse_type(parser);
            exigir_identificador(parser);
            exigir(parser, ")");
            parse_block(parser);
        }

        if (consumir(parser, "finally")) {
            parse_block(parser);
        }
        return;
    }

    if (consumir(parser, "assert")) {
        parse_expression(parser, 1);
        if (consumir(parser, ":")) {
            parse_expression(parser, 1);
        }
        exigir(parser, ";");
        return;
    }

    if (parece_declaracao(parser)) {
        parse_modifiers(parser);
        parse_var_declaration(parser, 1);
        return;
    }

    if (e_identificador(parser) && strcmp(lexema_relativo(parser, 1), ":") == 0) {
        avancar(parser);
        avancar(parser);
        parse_statement(parser);
        return;
    }

    parse_expression(parser, 1);
    exigir(parser, ";");
}

static void parse_params(Parser *parser) {
    exigir(parser, "(");

    if (!lexema_e(parser, ")")) {
        do {
            parse_modifiers(parser);
            parse_type(parser);
            consumir(parser, "...");
            exigir_identificador(parser);
            while (consumir(parser, "[")) {
                exigir(parser, "]");
            }
        } while (consumir(parser, ","));
    }

    exigir(parser, ")");
}

static void parse_member(Parser *parser, const char *nome_classe) {
    parse_modifiers(parser);

    if (lexema_e(parser, "}")) {
        return;
    }

    if (consumir(parser, ";")) {
        return;
    }

    if (lexema_e(parser, "class") ||
        lexema_e(parser, "interface") ||
        lexema_e(parser, "enum")) {
        avancar(parser);
        exigir_identificador(parser);
        while (!esta_no_fim(parser) && !lexema_e(parser, "{")) {
            avancar(parser);
        }
        if (lexema_e(parser, "{")) {
            parse_block(parser);
        }
        return;
    }

    if (lexema_e(parser, "{")) {
        parse_block(parser);
        return;
    }

    if (e_identificador(parser) &&
        strcmp(lexema_atual(parser), nome_classe) == 0 &&
        strcmp(lexema_relativo(parser, 1), "(") == 0) {
        avancar(parser);
        parse_params(parser);
        if (lexema_e(parser, "{")) {
            parse_block(parser);
        } else {
            exigir(parser, ";");
        }
        return;
    }

    if (!inicia_tipo(parser)) {
        registrar_erro_sintatico(parser, "Tipo ou membro esperado, encontrado '%s'", lexema_atual(parser));
        avancar(parser);
        return;
    }

    parse_type(parser);

    if (!exigir_identificador(parser)) {
        while (!esta_no_fim(parser) &&
               !lexema_e(parser, ";") &&
               !lexema_e(parser, "}")) {
            avancar(parser);
        }
        consumir(parser, ";");
        return;
    }

    if (lexema_e(parser, "(")) {
        parse_params(parser);

        if (consumir(parser, "throws")) {
            parse_qualified_name(parser);
            while (consumir(parser, ",")) {
                parse_qualified_name(parser);
            }
        }

        if (lexema_e(parser, "{")) {
            parse_block(parser);
        } else {
            exigir(parser, ";");
        }
    } else {
        parse_var_tail(parser);
        exigir(parser, ";");
    }
}

static void parse_class_decl(Parser *parser) {
    int tipo = 0;
    char nome[256] = "";
    int contador = 0;

    parse_modifiers(parser);

    if (consumir(parser, "class")) {
        tipo = 1;
    } else if (consumir(parser, "interface")) {
        tipo = 2;
    } else if (consumir(parser, "enum")) {
        tipo = 3;
    } else {
        registrar_erro_sintatico(parser, "Declaracao de classe, interface ou enum esperada");
        avancar(parser);
        return;
    }

    if (e_identificador(parser)) {
        snprintf(nome, sizeof(nome), "%s", lexema_atual(parser));
        avancar(parser);
    } else {
        exigir_identificador(parser);
    }

    parse_generic(parser);

    if (consumir(parser, "extends")) {
        parse_type(parser);
        while (consumir(parser, ",")) {
            parse_type(parser);
        }
    }

    if (consumir(parser, "implements")) {
        parse_type(parser);
        while (consumir(parser, ",")) {
            parse_type(parser);
        }
    }

    if (!exigir(parser, "{")) {
        return;
    }

    while (!esta_no_fim(parser) &&
           !lexema_e(parser, "}") &&
           contador++ < 100000) {
        int posicao_inicial = parser->posicao;

        if (tipo == 3 &&
            e_identificador(parser) &&
            (strcmp(lexema_relativo(parser, 1), ",") == 0 ||
             strcmp(lexema_relativo(parser, 1), ";") == 0 ||
             strcmp(lexema_relativo(parser, 1), "}") == 0)) {
            avancar(parser);
            if (!consumir(parser, ",")) {
                consumir(parser, ";");
            }
            continue;
        }

        parse_member(parser, nome);

        if (parser->posicao == posicao_inicial) {
            avancar(parser);
        }
    }

    exigir(parser, "}");
}

void analisar_sintaxe(const VetorTokens *tokens, ListaErrosSintaticos *erros) {
    Parser parser;

    memset(erros, 0, sizeof(*erros));

    if (tokens->total == 0) {
        return;
    }

    parser.tokens = tokens;
    parser.posicao = 0;
    parser.profundidade = 0;
    parser.erros = erros;

    if (consumir(&parser, "package")) {
        parse_qualified_name(&parser);
        exigir(&parser, ";");
    }

    while (consumir(&parser, "import")) {
        consumir(&parser, "static");
        exigir_identificador(&parser);

        while (consumir(&parser, ".")) {
            if (consumir(&parser, "*")) {
                break;
            }
            exigir_identificador(&parser);
        }

        exigir(&parser, ";");
    }

    while (!esta_no_fim(&parser)) {
        int posicao_inicial = parser.posicao;

        if (consumir(&parser, ";")) {
            continue;
        }

        parse_class_decl(&parser);

        if (parser.posicao == posicao_inicial) {
            avancar(&parser);
        }
    }
}

void erros_sintaticos_liberar(ListaErrosSintaticos *erros) {
    free(erros->itens);
    memset(erros, 0, sizeof(*erros));
}

/* A AST representa o subconjunto semantico, menor que a sintaxe reconhecida acima. */
static char *copiar_texto_ast(const char *texto) {
    size_t tamanho = strlen(texto) + 1;
    char *copia = malloc(tamanho);
    if (copia == NULL) {
        exit(EXIT_FAILURE);
    }
    memcpy(copia, texto, tamanho);
    return copia;
}

static int identificador_ast(Parser *parser) {
    return token_atual(parser)->tipo == TOK_IDENTIFICADOR || lexema_e(parser, "String");
}

static int tipo_primitivo_ast(Parser *parser) {
    return e_primitivo(parser) || lexema_e(parser, "String");
}

static void consumir_modificadores_ast(Parser *parser) {
    const char *modificadores[] = {
        "public", "private", "protected", "static", "final", "abstract", "volatile", "synchronized"
    };
    for (;;) {
        int encontrado = 0;
        for (size_t i = 0; i < sizeof(modificadores) / sizeof(modificadores[0]); i++) {
            if (lexema_e(parser, modificadores[i])) {
                encontrado = 1;
                break;
            }
        }
        if (!encontrado) {
            return;
        }
        avancar(parser);
    }
}

static char *ler_tipo_ast(Parser *parser) {
    if (!tipo_primitivo_ast(parser) && !identificador_ast(parser)) {
        registrar_erro_sintatico(parser, "Tipo esperado");
        return copiar_texto_ast("?");
    }

    char tipo[256];
    snprintf(tipo, sizeof(tipo), "%s", lexema_atual(parser));
    avancar(parser);
    while (lexema_e(parser, "[") && strcmp(lexema_relativo(parser, 1), "]") == 0) {
        avancar(parser);
        avancar(parser);
        if (strlen(tipo) + 2 < sizeof(tipo)) {
            strcat(tipo, "[]");
        }
    }
    return copiar_texto_ast(tipo);
}

static AST *novo_no_atual(Parser *parser, NodeKind categoria, const char *texto, const char *tipo) {
    const Token *token = token_atual(parser);
    return ast_new(categoria, texto, tipo, token->linha, token->coluna);
}

static int precedencia_ast(const char *operador) {
    static const PrecedenciaOperador operadores[] = {
        {"=", 1}, {"+=", 1}, {"-=", 1}, {"*=", 1}, {"/=", 1},
        {"||", 2}, {"&&", 3}, {"==", 4}, {"!=", 4},
        {"<", 5}, {">", 5}, {"<=", 5}, {">=", 5},
        {"+", 6}, {"-", 6}, {"*", 7}, {"/", 7}, {"%", 7}
    };
    for (size_t i = 0; i < sizeof(operadores) / sizeof(operadores[0]); i++) {
        if (strcmp(operador, operadores[i].operador) == 0) {
            return operadores[i].valor;
        }
    }
    return 0;
}

static const char *tipo_literal_ast(const Token *token) {
    size_t tamanho = strlen(token->lexema);
    char sufixo = tamanho ? (char)tolower((unsigned char)token->lexema[tamanho - 1]) : 0;
    switch (token->tipo) {
        case TOK_LITERAL_INTEIRO: return sufixo == 'l' ? "long" : "int";
        case TOK_LITERAL_DECIMAL: return sufixo == 'f' ? "float" : "double";
        case TOK_LITERAL_TEXTO: return "String";
        case TOK_LITERAL_CARACTERE: return "char";
        case TOK_LITERAL_BOOLEANO: return "boolean";
        default: return "null";
    }
}

static AST *construir_expressao(Parser *parser, int precedencia_minima);
static AST *construir_instrucao(Parser *parser);

static void construir_argumentos(Parser *parser, AST *chamada) {
    if (!lexema_e(parser, ")")) {
        ast_add(chamada, construir_expressao(parser, 1));
        while (consumir(parser, ",")) {
            ast_add(chamada, construir_expressao(parser, 1));
        }
    }
    exigir(parser, ")");
}

static AST *construir_primaria(Parser *parser) {
    if (!entrar_regra(parser)) {
        return novo_no_atual(parser, N_EMPTY, "erro", NULL);
    }

    const Token *token = token_atual(parser);
    AST *no;
    if (lexema_e(parser, "!") || lexema_e(parser, "-") || lexema_e(parser, "+") ||
        lexema_e(parser, "++") || lexema_e(parser, "--")) {
        no = novo_no_atual(parser, N_UNARY, token->lexema, NULL);
        avancar(parser);
        ast_add(no, construir_primaria(parser));
    } else if (consumir(parser, "(")) {
        no = construir_expressao(parser, 1);
        exigir(parser, ")");
    } else if (consumir(parser, "new")) {
        no = ast_new(N_NEW, "new", NULL, token->linha, token->coluna);
        no->type = ler_tipo_ast(parser);
        if (consumir(parser, "(")) {
            construir_argumentos(parser, no);
        } else if (consumir(parser, "[")) {
            if (!lexema_e(parser, "]")) {
                ast_add(no, construir_expressao(parser, 1));
            }
            exigir(parser, "]");
            char *tipo = realloc(no->type, strlen(no->type) + 3);
            if (tipo == NULL) {
                exit(EXIT_FAILURE);
            }
            no->type = tipo;
            strcat(no->type, "[]");
        }
    } else if (e_literal(parser)) {
        no = novo_no_atual(parser, N_LITERAL, token->lexema, tipo_literal_ast(token));
        avancar(parser);
    } else if (identificador_ast(parser) || lexema_e(parser, "this")) {
        no = novo_no_atual(parser, N_IDENT, token->lexema, NULL);
        avancar(parser);
    } else {
        registrar_erro_sintatico(parser, "Expressao invalida em '%s'", token->lexema);
        no = novo_no_atual(parser, N_EMPTY, "erro", NULL);
        avancar(parser);
    }

    while (!esta_no_fim(parser)) {
        if (consumir(parser, ".")) {
            if (!identificador_ast(parser)) {
                registrar_erro_sintatico(parser, "Membro esperado");
                break;
            }
            AST *membro = novo_no_atual(parser, N_MEMBER, lexema_atual(parser), NULL);
            avancar(parser);
            ast_add(membro, no);
            no = membro;
        } else if (consumir(parser, "(")) {
            AST *chamada = novo_no_atual(parser, N_CALL, "chamada", NULL);
            ast_add(chamada, no);
            construir_argumentos(parser, chamada);
            no = chamada;
        } else if (consumir(parser, "[")) {
            AST *indice = novo_no_atual(parser, N_INDEX, "[]", NULL);
            ast_add(indice, no);
            ast_add(indice, construir_expressao(parser, 1));
            exigir(parser, "]");
            no = indice;
        } else if (lexema_e(parser, "++") || lexema_e(parser, "--")) {
            AST *unario = novo_no_atual(parser, N_UNARY, lexema_atual(parser), NULL);
            avancar(parser);
            ast_add(unario, no);
            no = unario;
        } else {
            break;
        }
    }
    parser->profundidade--;
    return no;
}

static AST *construir_expressao(Parser *parser, int precedencia_minima) {
    if (!entrar_regra(parser)) {
        return novo_no_atual(parser, N_EMPTY, "erro", NULL);
    }
    AST *esquerda = construir_primaria(parser);
    while (!esta_no_fim(parser)) {
        int nivel = precedencia_ast(lexema_atual(parser));
        if (nivel < precedencia_minima) {
            break;
        }
        AST *operacao = novo_no_atual(parser, nivel == 1 ? N_ASSIGN : N_BINARY,
                                      lexema_atual(parser), NULL);
        avancar(parser);
        ast_add(operacao, esquerda);
        ast_add(operacao, construir_expressao(parser, nivel + (nivel == 1 ? 0 : 1)));
        esquerda = operacao;
    }
    parser->profundidade--;
    return esquerda;
}

static AST *construir_bloco(Parser *parser) {
    AST *bloco = novo_no_atual(parser, N_BLOCK, "{}", NULL);
    exigir(parser, "{");
    int contador = 0;
    while (!esta_no_fim(parser) && !lexema_e(parser, "}") && contador++ < 10000) {
        int posicao_inicial = parser->posicao;
        ast_add(bloco, construir_instrucao(parser));
        if (parser->posicao == posicao_inicial) {
            avancar(parser);
        }
    }
    exigir(parser, "}");
    return bloco;
}

static AST *construir_variavel(Parser *parser) {
    const Token *token = token_atual(parser);
    char *tipo = ler_tipo_ast(parser);
    AST *variavel = ast_new(N_VAR, NULL, tipo, token->linha, token->coluna);
    free(tipo);
    if (!identificador_ast(parser)) {
        registrar_erro_sintatico(parser, "Nome de variavel esperado");
        return variavel;
    }
    variavel->text = copiar_texto_ast(lexema_atual(parser));
    avancar(parser);
    if (consumir(parser, "=")) {
        ast_add(variavel, construir_expressao(parser, 1));
    }
    return variavel;
}

static AST *construir_for(Parser *parser) {
    AST *laco = novo_no_atual(parser, N_FOR, "for", NULL);
    avancar(parser);
    exigir(parser, "(");
    if (lexema_e(parser, ";")) {
        ast_add(laco, novo_no_atual(parser, N_EMPTY, "init", NULL));
    } else if (tipo_primitivo_ast(parser)) {
        ast_add(laco, construir_variavel(parser));
    } else {
        ast_add(laco, construir_expressao(parser, 1));
    }
    exigir(parser, ";");
    ast_add(laco, lexema_e(parser, ";") ? novo_no_atual(parser, N_EMPTY, "cond", NULL)
                                      : construir_expressao(parser, 1));
    exigir(parser, ";");
    ast_add(laco, lexema_e(parser, ")") ? novo_no_atual(parser, N_EMPTY, "passo", NULL)
                                      : construir_expressao(parser, 1));
    exigir(parser, ")");
    ast_add(laco, construir_instrucao(parser));
    return laco;
}

static AST *construir_instrucao_impl(Parser *parser);

static AST *construir_instrucao(Parser *parser) {
    if (!entrar_regra(parser)) {
        return novo_no_atual(parser, N_EMPTY, "erro", NULL);
    }
    AST *instrucao = construir_instrucao_impl(parser);
    parser->profundidade--;
    return instrucao;
}

static AST *construir_instrucao_impl(Parser *parser) {
    if (lexema_e(parser, "{")) {
        return construir_bloco(parser);
    }
    if (consumir(parser, ";")) {
        return novo_no_atual(parser, N_EMPTY, ";", NULL);
    }
    if (lexema_e(parser, "if") || lexema_e(parser, "while")) {
        int condicional = lexema_e(parser, "if");
        AST *no = novo_no_atual(parser, condicional ? N_IF : N_WHILE, lexema_atual(parser), NULL);
        avancar(parser);
        exigir(parser, "(");
        ast_add(no, construir_expressao(parser, 1));
        exigir(parser, ")");
        ast_add(no, construir_instrucao(parser));
        if (condicional && consumir(parser, "else")) {
            ast_add(no, construir_instrucao(parser));
        }
        return no;
    }
    if (lexema_e(parser, "for")) {
        return construir_for(parser);
    }
    if (lexema_e(parser, "return")) {
        AST *retorno = novo_no_atual(parser, N_RETURN, "return", NULL);
        avancar(parser);
        if (!lexema_e(parser, ";")) {
            ast_add(retorno, construir_expressao(parser, 1));
        }
        exigir(parser, ";");
        return retorno;
    }
    if (lexema_e(parser, "break") || lexema_e(parser, "continue")) {
        AST *no = novo_no_atual(parser, N_EMPTY, lexema_atual(parser), NULL);
        avancar(parser);
        exigir(parser, ";");
        return no;
    }

    if (tipo_primitivo_ast(parser)) {
        int posicao_inicial = parser->posicao;
        avancar(parser);
        while (consumir(parser, "[")) {
            exigir(parser, "]");
        }
        int declaracao = identificador_ast(parser);
        parser->posicao = posicao_inicial;
        if (declaracao) {
            AST *primeira = construir_variavel(parser);
            AST *ultima = primeira;
            while (consumir(parser, ",")) {
                AST *variavel = novo_no_atual(parser, N_VAR, lexema_atual(parser), primeira->type);
                avancar(parser);
                if (consumir(parser, "=")) {
                    ast_add(variavel, construir_expressao(parser, 1));
                }
                ultima->next = variavel;
                ultima = variavel;
            }
            exigir(parser, ";");
            return primeira;
        }
    }

    AST *instrucao = novo_no_atual(parser, N_EXPR_STMT, "expressao", NULL);
    ast_add(instrucao, construir_expressao(parser, 1));
    exigir(parser, ";");
    return instrucao;
}

static AST *construir_metodo(Parser *parser, const char *tipo, const char *nome,
                             int linha, int coluna) {
    AST *metodo = ast_new(N_METHOD, nome, tipo, linha, coluna);
    exigir(parser, "(");
    if (!lexema_e(parser, ")")) {
        do {
            const Token *token = token_atual(parser);
            char *tipo_parametro = ler_tipo_ast(parser);
            AST *parametro = ast_new(N_PARAM, identificador_ast(parser) ? lexema_atual(parser) : "?",
                                     tipo_parametro, token->linha, token->coluna);
            free(tipo_parametro);
            if (identificador_ast(parser)) {
                avancar(parser);
            } else {
                registrar_erro_sintatico(parser, "Parametro sem nome");
            }
            ast_add(metodo, parametro);
        } while (consumir(parser, ","));
    }
    exigir(parser, ")");
    if (lexema_e(parser, "{")) {
        ast_add(metodo, construir_bloco(parser));
    } else {
        exigir(parser, ";");
    }
    return metodo;
}

static void construir_membro(Parser *parser, AST *classe) {
    const Token *token = token_atual(parser);
    char *tipo = ler_tipo_ast(parser);
    if (!identificador_ast(parser)) {
        registrar_erro_sintatico(parser, "Nome de membro esperado");
        free(tipo);
        avancar(parser);
        return;
    }

    char *nome = copiar_texto_ast(lexema_atual(parser));
    avancar(parser);
    if (lexema_e(parser, "(")) {
        ast_add(classe, construir_metodo(parser, tipo, nome, token->linha, token->coluna));
    } else {
        AST *campo = ast_new(N_VAR, nome, tipo, token->linha, token->coluna);
        if (consumir(parser, "=")) {
            ast_add(campo, construir_expressao(parser, 1));
        }
        ast_add(classe, campo);
        while (consumir(parser, ",")) {
            campo = novo_no_atual(parser, N_VAR, lexema_atual(parser), tipo);
            avancar(parser);
            if (consumir(parser, "=")) {
                ast_add(campo, construir_expressao(parser, 1));
            }
            ast_add(classe, campo);
        }
        exigir(parser, ";");
    }
    free(nome);
    free(tipo);
}

static int ast_dentro_limite(const AST *no, int nivel) {
    if (no && nivel >= 128) {
        return 0;
    }
    for (; no; no = no->next) {
        if (!ast_dentro_limite(no->child, nivel + 1)) {
            return 0;
        }
    }
    return 1;
}

AST *construir_ast(const VetorTokens *tokens, ListaErrosSintaticos *erros) {
    memset(erros, 0, sizeof(*erros));
    if (tokens->total == 0) {
        return ast_new(N_PROGRAM, "Java", NULL, 1, 1);
    }
    Parser parser = {tokens, 0, 0, erros};
    AST *raiz = novo_no_atual(&parser, N_PROGRAM, "Java", NULL);
    while (!esta_no_fim(&parser)) {
        if (consumir(&parser, "package") || consumir(&parser, "import")) {
            while (!esta_no_fim(&parser) && !consumir(&parser, ";")) {
                avancar(&parser);
            }
            continue;
        }
        consumir_modificadores_ast(&parser);
        if (!consumir(&parser, "class")) {
            registrar_erro_sintatico(&parser, "Classe esperada");
            avancar(&parser);
            continue;
        }
        AST *classe = novo_no_atual(&parser, N_CLASS, lexema_atual(&parser), NULL);
        avancar(&parser);
        ast_add(raiz, classe);
        if (!exigir(&parser, "{")) {
            break;
        }

        int contador = 0;
        while (!esta_no_fim(&parser) && !lexema_e(&parser, "}") && contador++ < 10000) {
            consumir_modificadores_ast(&parser);
            if (consumir(&parser, ";")) {
                continue;
            }
            if (lexema_e(&parser, "}")) {
                break;
            }
            if (!tipo_primitivo_ast(&parser) && !identificador_ast(&parser)) {
                registrar_erro_sintatico(&parser, "Membro de classe invalido");
                avancar(&parser);
                continue;
            }
            construir_membro(&parser, classe);
        }
        exigir(&parser, "}");
    }
    if (!ast_dentro_limite(raiz, 0)) {
        registrar_erro_sintatico(&parser, "Limite de profundidade da AST excedido");
    }
    return raiz;
}
