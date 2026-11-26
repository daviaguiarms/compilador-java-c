# Revisão técnica e testes — 24/11/2026

> Registro histórico anterior à refatoração. A organização e a validação atuais
> estão em `documentacao/REVISAO_CRITICA.md`; as execuções descritas abaixo não foram
> todas repetidas na refatoração.

## Escopo e critério

Revisão do projeto de análise léxica, sintática e semântica em C, para o subconjunto definido de Java, comparando o código com o PDF oficial da disciplina Compiladores. Grupo: Bruno Trindade Friaça Moreira; Davi Aguiar Macedo Santos; Gustavo Costa Nascimento; Lucas Henrique de Souza e Sá. Apresentação: 26/11/2026. A professora autorizou quatro integrantes, segundo confirmação do grupo.

## Verificação objetiva

- Construção: `make clean && make`, GCC C11, opções `-Wall -Wextra -Wpedantic`. Compilou sem avisos.
- Regressão original: 6/6 casos aprovados.
- Casos de code review: 40/40 aprovados (26 inválidos e 14 válidos).
- Robustez: 150 entradas aleatórias, nenhum travamento, timeout ou código de retorno inesperado no ambiente de teste.
- Sanitizadores: execução de seis arquivos representativos com ASan e UBSan; nenhuma mensagem de alerta observada.
- Saída: tokens, tabela de símbolos lexicais, confirmação sintática, AST indentada, erros por etapa e linha/coluna.
- Entrega: Makefile, README, módulos `lexer`, `parser`, `ast`, `semantic`, `symbol_table`, `main`, exemplos, relatório PDF de 8 páginas e slides.

## Correções aplicadas

1. Impedir variáveis, campos e parâmetros `void`.
2. Verificar tipos e destinos de operadores `++` e `--`.
3. Rejeitar indexação de não-vetores e índices booleanos.
4. Validar que atribuições compostas aritméticas operam em números.
5. Impedir uso de nome de método sem chamada como valor.
6. Conferir `System.out.print` e `System.out.println` (nomes e quantidades de argumentos suportados).
7. Manter quatro integrantes no driver e no README.
8. Adicionar 40 testes extras reproduzíveis, e rodá-los em `make test`.

## Correspondência ao enunciado

- Léxico: scanner manual em C; palavras-chave, identificadores, literais, operadores, delimitadores, comentários; linha/coluna e tabela inicial.
- Sintático: parser manual descendente recursivo consumindo tokens, com gramática definida no relatório e erros sem travamentos nos testes.
- Semântico: AST percorrida com tipos, escopos, declarações anteriores ao uso, redeclarações, argumentos e retorno dentro do subconjunto documentado.
- Projeto: C11, módulos e Makefile; recebe arquivo por argumento; imprime resultados de análise.
- Materiais: relatório técnico (8 páginas), slides, testes e instruções.

## Limitações que NÃO foram eliminadas

Apenas uma parte de Java é implementada. A análise semântica não cobre completamente construtores, `switch`, exceções, lambdas, generics, expressões com bibliotecas externas, métodos sobrecarregados, resolução de imports, hierarquias de classes, controle de fluxos de retorno e todas as regras de promoção de tipos. O parser aceita algumas estruturas que a AST semântica rejeitará. Portanto, testes novos da professora que explorem recursos fora deste subconjunto poderão falhar. O compilador não gera bytecode. A afirmação correta é **aderência ao subconjunto explicitado, não conformidade total com Java**.

## Exigências de responsabilidade do grupo

Os quatro integrantes devem demonstrar entendimento do código e participar da apresentação. Cada integrante deve realizar commits próprios reais no repositório GitHub/GitLab, e a declaração de apoio de IA precisa permanecer no relatório. Não foram criados commits, nem um repositório remoto, durante esta revisão.

## Comandos

```sh
make clean
make
make test
./compilador testes/Valido.java
./compilador testes/ErroSintatico.java
./compilador testes/ErroSemantico.java
```
