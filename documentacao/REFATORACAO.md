# Refatoração — 26/11/2026

> Registro da primeira refatoração. A revisão crítica posterior, suas correções e
> os resultados atuais estão em `REVISAO_CRITICA.md`.

## Referência e organização

O documento principal foi `Enunciado_Oficial.pdf`, especialmente as seções 2 e 3.
Os arquivos C continuam na raiz, como previsto no enunciado. Não foram adicionadas
bibliotecas nem ampliado o subconjunto de Java.

| Critério | Onde está implementado |
| --- | --- |
| Scanner manual, tokens, comentários e erros com posição | `lexer.c` |
| Tabela inicial de identificadores, com hash próprio | `symbol_table.c` |
| Parser descendente recursivo e diagnósticos sintáticos | `parser.c` |
| Construção da AST do subconjunto | Funções `construir_*` em `parser.c` |
| Representação, impressão indentada e liberação da AST | `ast.c` |
| Declarações, tipos, escopos e argumentos de chamadas | `semantic.c` |
| Entrada por argumento, participantes e saída das etapas | `main.c` |
| Compilação C11, dependências de cabeçalhos e testes | `Makefile` |

## Alterações principais

- O parser chamava `registrar_erro`, do lexer, com argumentos incompatíveis.
  As chamadas passaram a usar `registrar_erro_sintatico`, que já existia.
- O programa principal foi dividido em leitura, coleta de tokens, impressão e
  execução das análises. As mensagens e códigos de retorno foram mantidos.
- A construção da AST saiu de `semantic.c` e passou a reutilizar o cursor e os
  diagnósticos de `parser.c`. A validação sintática mais ampla foi preservada.
- O módulo semântico passou a ter funções legíveis para declarações, chamadas,
  operações e escopos, em vez de funções inteiras compactadas em uma linha.
- O percurso distingue uma instrução de uma lista de instruções. Isso elimina
  cópias temporárias de nós e evita percorrer condição, passo e corpo de `for`
  duas vezes quando a inicialização é uma declaração.
- Os dois scripts de teste compartilham a execução dos casos. O Makefile usa o
  nome correto do executável no Windows e recompila quando cabeçalhos mudam.

## Validação

- Compilação com GCC/MinGW C11 e `-Wall -Wextra -Wpedantic -Werror -O2`.
- Os 46 casos existentes foram comparados com um executável de referência
  produzido após corrigir somente as chamadas de erro do parser. Todos mantiveram
  o retorno e a saída completa idênticos.
- Foram adicionados seis casos de regressão para escopos, laços e argumentos.
- Resultado final: 52/52 arquivos com o retorno esperado; nos seis exemplos de
  demonstração, a mensagem final também foi conferida. Uso sem argumento e
  arquivo inexistente retornaram `1`, como esperado.
- Neste ambiente, Python não está instalado. A execução dos arquivos Java foi
  feita diretamente pelo PowerShell; o comando `make test` requer Python 3.
- Os resultados históricos de sanitizadores e entradas aleatórias em
  `REVISAO_TECNICA.md` não representam novas execuções nesta refatoração.

## Entrega acadêmica

Na primeira refatoração, o relatório PDF e a apresentação não foram atualizados.
Essa pendência foi tratada na revisão crítica: há um novo relatório de 8 páginas
em `Relatorio_Tecnico_Atualizado.pdf` e um roteiro em `APRESENTACAO.md`. O PDF antigo
foi preservado. O grupo ainda precisa revisar os materiais e realizar contribuições
reais no GitHub/GitLab. Nenhum commit foi criado naquela primeira etapa;
a publicação posterior deve usar somente a identidade real configurada no Git.
