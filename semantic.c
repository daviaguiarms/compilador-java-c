#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { SIMBOLO_VARIAVEL, SIMBOLO_METODO } CategoriaSimbolo;

typedef struct {
    char *nome;
    char *tipo;
    int escopo;
    CategoriaSimbolo categoria;
    int total_parametros;
    char **parametros;
} SimboloSemantico;

typedef struct {
    SimboloSemantico *simbolos;
    int total;
    int capacidade;
    int escopo;
    SemanticErrors *erros;
    const char *tipo_retorno;
    const char *nome_classe;
} ContextoSemantico;

static char *copiar_texto(const char *texto) {
    size_t tamanho = strlen(texto) + 1;
    char *copia = malloc(tamanho);
    if (copia == NULL) {
        exit(EXIT_FAILURE);
    }
    memcpy(copia, texto, tamanho);
    return copia;
}

static void erro_semantico(SemanticErrors *erros, int linha, int coluna,
                           const char *formato, ...) {
    if (erros->count >= 100) {
        return;
    }
    if (erros->count == erros->cap) {
        erros->cap = erros->cap ? erros->cap * 2 : 16;
        SemanticError *itens = realloc(erros->items, (size_t)erros->cap * sizeof(*itens));
        if (itens == NULL) {
            exit(EXIT_FAILURE);
        }
        erros->items = itens;
    }

    SemanticError *erro = &erros->items[erros->count++];
    erro->line = linha;
    erro->col = coluna;
    va_list argumentos;
    va_start(argumentos, formato);
    vsnprintf(erro->msg, sizeof(erro->msg), formato, argumentos);
    va_end(argumentos);
}

static SimboloSemantico *buscar_simbolo(ContextoSemantico *contexto, const char *nome) {
    for (int i = contexto->total - 1; i >= 0; i--) {
        if (strcmp(contexto->simbolos[i].nome, nome) == 0) {
            return &contexto->simbolos[i];
        }
    }
    return NULL;
}

static SimboloSemantico *buscar_membro(ContextoSemantico *contexto, const char *nome) {
    for (int i = contexto->total - 1; i >= 0; i--) {
        SimboloSemantico *simbolo = &contexto->simbolos[i];
        if (simbolo->escopo == 1 && strcmp(simbolo->nome, nome) == 0) {
            return simbolo;
        }
    }
    return NULL;
}

static int membro_da_classe(ContextoSemantico *contexto, const AST *no) {
    const AST *objeto = no ? no->child : NULL;
    return objeto && objeto->kind == N_IDENT &&
           (strcmp(objeto->text, "this") == 0 ||
            (contexto->nome_classe && strcmp(objeto->text, contexto->nome_classe) == 0));
}

static SimboloSemantico *declarar_simbolo(ContextoSemantico *contexto, const AST *no,
                                         CategoriaSimbolo categoria) {
    const char *nome = no->text ? no->text : "?";
    for (int i = contexto->total - 1; i >= 0; i--) {
        SimboloSemantico *simbolo = &contexto->simbolos[i];
        if (simbolo->escopo < contexto->escopo) {
            break;
        }
        if (strcmp(simbolo->nome, nome) == 0) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "Redeclaracao de '%s' no mesmo escopo", nome);
            return NULL;
        }
    }

    if (contexto->total == contexto->capacidade) {
        contexto->capacidade = contexto->capacidade ? contexto->capacidade * 2 : 32;
        SimboloSemantico *simbolos = realloc(contexto->simbolos,
            (size_t)contexto->capacidade * sizeof(*simbolos));
        if (simbolos == NULL) {
            exit(EXIT_FAILURE);
        }
        contexto->simbolos = simbolos;
    }

    SimboloSemantico *simbolo = &contexto->simbolos[contexto->total++];
    memset(simbolo, 0, sizeof(*simbolo));
    simbolo->nome = copiar_texto(nome);
    simbolo->tipo = copiar_texto(no->type ? no->type : "?");
    simbolo->categoria = categoria;
    simbolo->escopo = contexto->escopo;
    return simbolo;
}

static void sair_escopo(ContextoSemantico *contexto) {
    while (contexto->total > 0 &&
           contexto->simbolos[contexto->total - 1].escopo == contexto->escopo) {
        SimboloSemantico *simbolo = &contexto->simbolos[--contexto->total];
        for (int i = 0; i < simbolo->total_parametros; i++) {
            free(simbolo->parametros[i]);
        }
        free(simbolo->parametros);
        free(simbolo->nome);
        free(simbolo->tipo);
    }
    contexto->escopo--;
}

static int tipo_numerico(const char *tipo) {
    const char *tipos[] = {"int", "double", "float", "long", "short", "byte", "char"};
    for (size_t i = 0; i < sizeof(tipos) / sizeof(tipos[0]); i++) {
        if (strcmp(tipo, tipos[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int tipo_void(const char *tipo) {
    return tipo && strncmp(tipo, "void", 4) == 0 &&
           (tipo[4] == '\0' || tipo[4] == '[');
}

static int tipos_compativeis(const char *destino, const char *origem) {
    if (destino == NULL || origem == NULL ||
        strcmp(destino, "?") == 0 || strcmp(origem, "?") == 0) {
        return 1;
    }
    if (strcmp(destino, origem) == 0) {
        return 1;
    }
    if (strcmp(origem, "null") == 0) {
        return !tipo_numerico(destino) && strcmp(destino, "boolean") != 0 &&
               strcmp(destino, "void") != 0;
    }
    if (!tipo_numerico(destino) || !tipo_numerico(origem)) {
        return 0;
    }
    if (strcmp(destino, "double") == 0) {
        return 1;
    }
    if (strcmp(destino, "float") == 0) {
        return strcmp(origem, "double") != 0;
    }
    if (strcmp(destino, "long") == 0 && strcmp(origem, "int") == 0) {
        return 1;
    }
    if (strcmp(destino, "long") == 0 || strcmp(destino, "int") == 0) {
        return strcmp(origem, "short") == 0 || strcmp(origem, "byte") == 0 ||
               strcmp(origem, "char") == 0;
    }
    return 0;
}

static int numero_ou_desconhecido(const char *tipo) {
    return tipo_numerico(tipo) || strcmp(tipo, "?") == 0;
}

static int booleano_ou_desconhecido(const char *tipo) {
    return strcmp(tipo, "boolean") == 0 || strcmp(tipo, "?") == 0;
}

static int destino_modificavel(ContextoSemantico *contexto, const AST *no) {
    if (no && no->kind == N_MEMBER && strcmp(no->text, "length") == 0 &&
        !membro_da_classe(contexto, no)) {
        return 0;
    }
    return no && (no->kind == N_IDENT || no->kind == N_MEMBER || no->kind == N_INDEX);
}

static const char *tipo_expressao(ContextoSemantico *contexto, AST *no);

static int tipo_indice(const char *tipo) {
    return strcmp(tipo, "?") == 0 || strcmp(tipo, "int") == 0 ||
           strcmp(tipo, "short") == 0 || strcmp(tipo, "byte") == 0 ||
           strcmp(tipo, "char") == 0;
}

static int chamada_de_saida(const AST *destino) {
    if (destino == NULL || destino->kind != N_MEMBER) {
        return 0;
    }
    const AST *objeto = destino->child;
    return objeto && objeto->kind == N_MEMBER && strcmp(objeto->text, "out") == 0 &&
           objeto->child && objeto->child->kind == N_IDENT &&
           strcmp(objeto->child->text, "System") == 0;
}

static const char *verificar_chamada(ContextoSemantico *contexto, AST *no) {
    AST *destino = no->child;
    AST *argumentos = destino ? destino->next : NULL;
    const char *nome = destino ? destino->text : "?";
    int quantidade = 0;
    for (AST *argumento = argumentos; argumento; argumento = argumento->next) {
        quantidade++;
    }

    if (chamada_de_saida(destino)) {
        for (AST *argumento = argumentos; argumento; argumento = argumento->next) {
            if (strcmp(tipo_expressao(contexto, argumento), "void") == 0) {
                erro_semantico(contexto->erros, argumento->line, argumento->col,
                               "Argumento de System.out.%s nao pode ser void", nome);
            }
        }
        if (strcmp(nome, "print") != 0 && strcmp(nome, "println") != 0) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "Metodo System.out.%s nao suportado", nome);
        } else if (quantidade > 1 || (strcmp(nome, "print") == 0 && quantidade != 1)) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "System.out.%s recebeu %d argumento(s)", nome, quantidade);
        }
        return "void";
    }

    SimboloSemantico *metodo = NULL;
    int chamada_local = destino && destino->kind == N_IDENT;
    if (destino && destino->kind == N_IDENT) {
        metodo = buscar_simbolo(contexto, nome);
    } else if (destino && destino->kind == N_MEMBER) {
        if (membro_da_classe(contexto, destino)) {
            metodo = buscar_membro(contexto, nome);
            chamada_local = 1;
        } else {
            tipo_expressao(contexto, destino->child);
            erro_semantico(contexto->erros, no->line, no->col,
                           "Chamada de membro '%s' fora do subconjunto", nome);
        }
    } else {
        tipo_expressao(contexto, destino);
        erro_semantico(contexto->erros, no->line, no->col,
                       "Destino de chamada nao e uma funcao");
    }

    if (metodo == NULL) {
        if (chamada_local) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "Funcao '%s' nao declarada", nome);
        }
        for (AST *argumento = argumentos; argumento; argumento = argumento->next) {
            tipo_expressao(contexto, argumento);
        }
        return "?";
    }
    if (metodo->categoria != SIMBOLO_METODO) {
        erro_semantico(contexto->erros, no->line, no->col, "'%s' nao e uma funcao", nome);
        for (AST *argumento = argumentos; argumento; argumento = argumento->next) {
            tipo_expressao(contexto, argumento);
        }
        return "?";
    }
    if (quantidade != metodo->total_parametros) {
        erro_semantico(contexto->erros, no->line, no->col,
                       "Funcao '%s' espera %d argumento(s), recebeu %d",
                       nome, metodo->total_parametros, quantidade);
    }

    int indice = 0;
    for (AST *argumento = argumentos; argumento; argumento = argumento->next, indice++) {
        const char *tipo = tipo_expressao(contexto, argumento);
        if (indice < metodo->total_parametros &&
            !tipos_compativeis(metodo->parametros[indice], tipo)) {
            erro_semantico(contexto->erros, argumento->line, argumento->col,
                           "Argumento %d de '%s': esperado %s, recebido %s",
                           indice + 1, nome, metodo->parametros[indice], tipo);
        }
    }
    return metodo->tipo;
}

static const char *verificar_binaria(ContextoSemantico *contexto, AST *no) {
    const char *esquerda = tipo_expressao(contexto, no->child);
    const char *direita = tipo_expressao(contexto, no->child->next);
    const char *operador = no->text;

    if (strcmp(esquerda, "void") == 0 || strcmp(direita, "void") == 0) {
        erro_semantico(contexto->erros, no->line, no->col,
                       "Operador %s nao aceita expressao void", operador);
        return "?";
    }

    if (strcmp(operador, "==") == 0 || strcmp(operador, "!=") == 0) {
        if (!tipos_compativeis(esquerda, direita) && !tipos_compativeis(direita, esquerda)) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "Comparacao incompativel entre %s e %s", esquerda, direita);
        }
        return "boolean";
    }
    if (strcmp(operador, "<") == 0 || strcmp(operador, ">") == 0 ||
        strcmp(operador, "<=") == 0 || strcmp(operador, ">=") == 0) {
        if (!numero_ou_desconhecido(esquerda) || !numero_ou_desconhecido(direita)) {
            erro_semantico(contexto->erros, no->line, no->col,
                           "Comparacao relacional exige numeros");
        }
        return "boolean";
    }
    if (strcmp(operador, "&&") == 0 || strcmp(operador, "||") == 0) {
        if (!booleano_ou_desconhecido(esquerda) || !booleano_ou_desconhecido(direita)) {
            erro_semantico(contexto->erros, no->line, no->col, "Operador logico exige boolean");
        }
        return "boolean";
    }
    if (strcmp(operador, "+") == 0 &&
        (strcmp(esquerda, "String") == 0 || strcmp(direita, "String") == 0)) {
        return "String";
    }
    if (!numero_ou_desconhecido(esquerda) || !numero_ou_desconhecido(direita)) {
        erro_semantico(contexto->erros, no->line, no->col, "Operador aritmetico exige numeros");
    }
    if (strcmp(esquerda, "double") == 0 || strcmp(direita, "double") == 0) {
        return "double";
    }
    if (strcmp(esquerda, "float") == 0 || strcmp(direita, "float") == 0) {
        return "float";
    }
    if (strcmp(esquerda, "long") == 0 || strcmp(direita, "long") == 0) {
        return "long";
    }
    return "int";
}

static const char *tipo_expressao(ContextoSemantico *contexto, AST *no) {
    if (no == NULL) {
        return "void";
    }
    AST *primeiro = no->child;
    AST *segundo = primeiro ? primeiro->next : NULL;

    switch (no->kind) {
        case N_LITERAL:
            return no->type ? no->type : "?";
        case N_NEW: {
            const char *tipo = no->type ? no->type : "?";
            size_t tamanho = strlen(tipo);
            int vetor = tamanho > 2 && strcmp(tipo + tamanho - 2, "[]") == 0;
            if (tipo_void(tipo) || (!vetor && (tipo_numerico(tipo) || strcmp(tipo, "boolean") == 0))) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Tipo %s invalido na criacao com new", tipo);
            }
            if (vetor && primeiro == NULL) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Criacao de vetor exige tamanho");
            }
            for (AST *argumento = primeiro; argumento; argumento = argumento->next) {
                const char *recebido = tipo_expressao(contexto, argumento);
                if (vetor && !tipo_indice(recebido)) {
                    erro_semantico(contexto->erros, argumento->line, argumento->col,
                                   "Tamanho de vetor deve ser inteiro, recebido %s", recebido);
                }
            }
            return tipo;
        }
        case N_IDENT: {
            if (strcmp(no->text, "this") == 0) {
                return "?";
            }
            SimboloSemantico *simbolo = buscar_simbolo(contexto, no->text);
            if (simbolo == NULL) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Identificador '%s' usado antes da declaracao", no->text);
                return "?";
            }
            if (simbolo->categoria == SIMBOLO_METODO) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Metodo '%s' usado como valor sem chamada", no->text);
                return "?";
            }
            return simbolo->tipo;
        }
        case N_MEMBER: {
            if (membro_da_classe(contexto, no)) {
                SimboloSemantico *membro = buscar_membro(contexto, no->text);
                if (membro == NULL) {
                    erro_semantico(contexto->erros, no->line, no->col,
                                   "Membro '%s' nao declarado na classe", no->text);
                    return "?";
                }
                if (membro->categoria == SIMBOLO_METODO) {
                    erro_semantico(contexto->erros, no->line, no->col,
                                   "Metodo '%s' usado como valor sem chamada", no->text);
                    return "?";
                }
                return membro->tipo;
            }
            if (primeiro && primeiro->kind == N_IDENT && strcmp(primeiro->text, "System") == 0) {
                return "builtin";
            }
            const char *tipo = tipo_expressao(contexto, primeiro);
            if (strcmp(no->text, "length") == 0) {
                size_t tamanho = strlen(tipo);
                if (strcmp(tipo, "?") != 0 &&
                    !(tamanho > 2 && strcmp(tipo + tamanho - 2, "[]") == 0)) {
                    erro_semantico(contexto->erros, no->line, no->col,
                                   "length exige vetor, recebido %s", tipo);
                }
                return "int";
            }
            if (strcmp(tipo, "builtin") == 0) {
                return "builtin";
            }
            erro_semantico(contexto->erros, no->line, no->col,
                           "Acesso ao membro '%s' fora do subconjunto", no->text);
            return "?";
        }
        case N_INDEX: {
            const char *tipo = tipo_expressao(contexto, primeiro);
            const char *indice = tipo_expressao(contexto, segundo);
            size_t tamanho = strlen(tipo);
            if (!tipo_indice(indice)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Indice de vetor deve ser inteiro, recebido %s", indice);
            }
            if (tamanho > 2 && strcmp(tipo + tamanho - 2, "[]") == 0) {
                free(no->inferred_type);
                no->inferred_type = copiar_texto(tipo);
                no->inferred_type[tamanho - 2] = '\0';
                return no->inferred_type;
            }
            if (strcmp(tipo, "?") != 0) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Indexacao exige vetor, recebido %s", tipo);
            }
            return "?";
        }
        case N_CALL:
            return verificar_chamada(contexto, no);
        case N_BINARY:
            return verificar_binaria(contexto, no);
        case N_UNARY: {
            const char *tipo = tipo_expressao(contexto, primeiro);
            if (strcmp(no->text, "!") == 0) {
                if (!booleano_ou_desconhecido(tipo)) {
                    erro_semantico(contexto->erros, no->line, no->col, "Operador ! exige boolean");
                }
                return "boolean";
            }
            if (strcmp(no->text, "++") == 0 || strcmp(no->text, "--") == 0) {
                if (primeiro && !destino_modificavel(contexto, primeiro)) {
                    erro_semantico(contexto->erros, no->line, no->col,
                                   "Incremento/decremento exige variavel modificavel");
                }
                if (!numero_ou_desconhecido(tipo)) {
                    erro_semantico(contexto->erros, no->line, no->col,
                                   "Incremento/decremento exige tipo numerico, recebido %s", tipo);
                }
            } else if ((strcmp(no->text, "+") == 0 || strcmp(no->text, "-") == 0) &&
                       !numero_ou_desconhecido(tipo)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Operador unario aritmetico exige numero, recebido %s", tipo);
            }
            if ((strcmp(no->text, "+") == 0 || strcmp(no->text, "-") == 0) &&
                (strcmp(tipo, "char") == 0 || strcmp(tipo, "byte") == 0 || strcmp(tipo, "short") == 0)) {
                return "int";
            }
            return tipo;
        }
        case N_ASSIGN: {
            const char *destino = tipo_expressao(contexto, primeiro);
            const char *origem = tipo_expressao(contexto, segundo);
            if (primeiro && !destino_modificavel(contexto, primeiro)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Atribuicao exige destino modificavel");
            }
            if (strcmp(no->text, "=") != 0 &&
                (!numero_ou_desconhecido(destino) || !numero_ou_desconhecido(origem))) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Atribuicao composta %s exige numeros", no->text);
            }
            if (!tipos_compativeis(destino, origem)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Atribuicao incompativel: %s recebe %s", destino, origem);
            }
            return destino;
        }
        default:
            return "?";
    }
}

static void visitar_instrucao(ContextoSemantico *contexto, const AST *no);

static void visitar_lista(ContextoSemantico *contexto, const AST *no) {
    for (; no; no = no->next) {
        visitar_instrucao(contexto, no);
    }
}

static void visitar_em_escopo(ContextoSemantico *contexto, const AST *no) {
    contexto->escopo++;
    visitar_instrucao(contexto, no);
    sair_escopo(contexto);
}

static void verificar_condicao(ContextoSemantico *contexto, AST *condicao,
                               const AST *instrucao) {
    const char *tipo = tipo_expressao(contexto, condicao);
    if (booleano_ou_desconhecido(tipo)) {
        return;
    }
    if (instrucao->kind == N_IF) {
        erro_semantico(contexto->erros, instrucao->line, instrucao->col,
                       "Condicao de if deve ser boolean, recebeu %s", tipo);
    } else if (instrucao->kind == N_WHILE) {
        erro_semantico(contexto->erros, instrucao->line, instrucao->col,
                       "Condicao de while deve ser boolean");
    } else {
        erro_semantico(contexto->erros, condicao->line, condicao->col,
                       "Condicao de for deve ser boolean");
    }
}

static void verificar_tipo_declarado(ContextoSemantico *contexto, const AST *no,
                                    const char *categoria) {
    if (tipo_void(no->type)) {
        erro_semantico(contexto->erros, no->line, no->col,
                       "%s '%s' nao pode ter tipo void", categoria, no->text ? no->text : "?");
    }
}

static void visitar_instrucao(ContextoSemantico *contexto, const AST *no) {
    if (no == NULL) {
        return;
    }
    switch (no->kind) {
        case N_BLOCK:
            contexto->escopo++;
            visitar_lista(contexto, no->child);
            sair_escopo(contexto);
            break;
        case N_VAR: {
            verificar_tipo_declarado(contexto, no, "Variavel");
            const char *tipo = no->child ? tipo_expressao(contexto, no->child) : NULL;
            declarar_simbolo(contexto, no, SIMBOLO_VARIAVEL);
            if (tipo && !tipos_compativeis(no->type, tipo)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Inicializacao de '%s': %s nao aceita %s", no->text, no->type, tipo);
            }
            break;
        }
        case N_IF:
        case N_WHILE: {
            AST *condicao = no->child;
            verificar_condicao(contexto, condicao, no);
            const AST *corpo = condicao ? condicao->next : NULL;
            visitar_em_escopo(contexto, corpo);
            if (no->kind == N_IF && corpo && corpo->next) {
                visitar_em_escopo(contexto, corpo->next);
            }
            break;
        }
        case N_FOR: {
            AST *inicio = no->child;
            AST *condicao = inicio ? inicio->next : NULL;
            AST *passo = condicao ? condicao->next : NULL;
            AST *corpo = passo ? passo->next : NULL;
            contexto->escopo++;
            if (inicio && inicio->kind == N_VAR) {
                visitar_instrucao(contexto, inicio);
            } else if (inicio && inicio->kind != N_EMPTY) {
                tipo_expressao(contexto, inicio);
            }
            if (condicao && condicao->kind != N_EMPTY) {
                verificar_condicao(contexto, condicao, no);
            }
            if (passo && passo->kind != N_EMPTY) {
                tipo_expressao(contexto, passo);
            }
            visitar_instrucao(contexto, corpo);
            sair_escopo(contexto);
            break;
        }
        case N_RETURN: {
            const char *tipo = no->child ? tipo_expressao(contexto, no->child) : "void";
            const char *retorno = contexto->tipo_retorno ? contexto->tipo_retorno : "void";
            if ((no->child && strcmp(retorno, "void") == 0) || !tipos_compativeis(retorno, tipo)) {
                erro_semantico(contexto->erros, no->line, no->col,
                               "Retorno incompativel: metodo retorna %s, expressao %s", retorno, tipo);
            }
            break;
        }
        case N_EXPR_STMT:
            tipo_expressao(contexto, no->child);
            break;
        default:
            break;
    }
}

static void registrar_membros(ContextoSemantico *contexto, const AST *classe) {
    for (const AST *no = classe->child; no; no = no->next) {
        if (no->kind == N_METHOD) {
            SimboloSemantico *metodo = declarar_simbolo(contexto, no, SIMBOLO_METODO);
            if (metodo == NULL) {
                continue;
            }
            for (const AST *parametro = no->child;
                 parametro && parametro->kind == N_PARAM; parametro = parametro->next) {
                char **parametros = realloc(metodo->parametros,
                    (size_t)(metodo->total_parametros + 1) * sizeof(*parametros));
                if (parametros == NULL) {
                    exit(EXIT_FAILURE);
                }
                metodo->parametros = parametros;
                metodo->parametros[metodo->total_parametros++] = copiar_texto(parametro->type);
            }
        } else if (no->kind == N_VAR) {
            verificar_tipo_declarado(contexto, no, "Campo");
            declarar_simbolo(contexto, no, SIMBOLO_VARIAVEL);
        }
    }
}

void semantic_check(AST *root, SemanticErrors *errors) {
    ContextoSemantico contexto = {0};
    contexto.erros = errors;

    for (const AST *classe = root ? root->child : NULL; classe; classe = classe->next) {
        contexto.escopo++;
        contexto.nome_classe = classe->text;
        registrar_membros(&contexto, classe);
        for (const AST *no = classe->child; no; no = no->next) {
            if (no->kind == N_METHOD) {
                contexto.escopo++;
                contexto.tipo_retorno = no->type;
                const AST *parametro = no->child;
                while (parametro && parametro->kind == N_PARAM) {
                    verificar_tipo_declarado(&contexto, parametro, "Parametro");
                    declarar_simbolo(&contexto, parametro, SIMBOLO_VARIAVEL);
                    parametro = parametro->next;
                }
                visitar_lista(&contexto, parametro);
                sair_escopo(&contexto);
            } else if (no->kind == N_VAR && no->child) {
                const char *tipo = tipo_expressao(&contexto, no->child);
                if (!tipos_compativeis(no->type, tipo)) {
                    erro_semantico(errors, no->line, no->col, "Campo '%s' incompativel", no->text);
                }
            }
        }
        sair_escopo(&contexto);
    }

    while (contexto.escopo >= 0) {
        sair_escopo(&contexto);
    }
    free(contexto.simbolos);
}

void semantic_errors_free(SemanticErrors *errors) {
    free(errors->items);
    memset(errors, 0, sizeof(*errors));
}
