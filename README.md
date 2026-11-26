# Analisador Java (subconjunto) implementado em C

Disciplina: Compiladores — Professora Amanda Danielle Lima de Oliveira

Repositório: https://github.com/daviaguiarms/compilador-java-c

Grupo de Java (apresentação 26/11/2026):
- Bruno Trindade Friaça Moreira
- Davi Aguiar Macedo Santos
- Gustavo Costa Nascimento
- Lucas Henrique de Souza e Sá

## Objetivo

Implementar manualmente, em C11, as análises léxica, sintática e semântica para um **subconjunto didático da linguagem Java**. Não gera bytecode e não substitui `javac`.

## Compilar e executar

É necessário GCC/Clang e `make`. Python 3 é usado apenas nos testes automatizados. No terminal, a partir desta pasta:

```bash
make
./compilador testes/Valido.java
./compilador testes/ErroLexico.java
./compilador testes/ErroSintatico.java
./compilador testes/ErroSemantico.java
make test
```

No Windows com MinGW, use `mingw32-make` no lugar de `make`. Se o Python estiver instalado com outro nome, informe-o, por exemplo: `mingw32-make test PYTHON=py`.

Também é possível compilar e executar sem `make` e sem Python, no PowerShell:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic main.c lexer.c parser.c semantic.c ast.c symbol_table.c -o compilador.exe
.\compilador.exe testes\Valido.java
```

Retorno: `0` válido; `1` erro de uso/arquivo; `2` erro léxico; `3` erro sintático; `4` erro semântico.

### Eclipse CDT no Windows

Atualize o projeto com **F5**, compile em **Project > Build Project** e abra
**Run > Run Configurations... > C/C++ Application > Compilador_Valido**.
O arquivo `Compilador_Valido.launch` já informa `compilador.exe`, o argumento
`testes/Valido.java` e a pasta de trabalho do projeto. Não use a configuração
automática Core Build chamada apenas `compilador`, que pode gerar
`buildConfig is null` neste projeto Makefile. Mais detalhes e ajustes do
indexador em `documentacao/ECLIPSE.md`.

## Organização

- `main.c`: leitura do arquivo, nomes dos participantes, execução das etapas e impressão dos resultados.
- `lexer.c/.h`: scanner manual sem Lex/Flex, posição dos tokens e erros léxicos.
- `symbol_table.c/.h`: tabela inicial de identificadores lexicais.
- `parser.c/.h`: parser descendente recursivo para validar a sintaxe e construir a AST do subconjunto.
- `ast.c/.h`: nós da árvore, ligação entre filhos, impressão indentada e liberação de memória.
- `semantic.c/.h`: percurso da AST, tabela de declarações, escopos e validações de tipos e chamadas.
- `testes/`: entradas válidas e inválidas e script de regressão.
- `Makefile`: build e testes.

## Recursos reconhecidos na fase semântica

Classes simples, campos, métodos estáticos ou de instância, parâmetros, tipos primitivos, `String`, variáveis locais, vetores unidimensionais básicos, chamadas de métodos da classe corrente, `System.out.print/println`, expressões aritméticas/relacionais/lógicas, `if/else`, `while`, `for`, `return` e blocos.

As verificações incluem uso de variável não declarada, redeclaração no mesmo escopo, incompatibilidade de tipos, retorno, condições booleanas, escopos aninhados e aridade/tipo de argumentos de métodos da classe corrente. As chamadas podem usar nome simples, `this.metodo(...)` ou `NomeDaClasse.metodo(...)`. Campos de classes diferentes não compartilham escopo. A tabela léxica e a tabela semântica são diferentes: a primeira registra ocorrências e a segunda associa declarações a tipos e escopos.

O fluxo é: arquivo → tokens e tabela léxica → validação sintática → AST → verificação semântica. Uma etapa com erros impede a próxima. A construção da AST usa as mesmas rotinas básicas de leitura de tokens do parser. Construções reconhecidas pela sintaxe, mas fora do subconjunto da AST, continuam sendo informadas na etapa semântica, com retorno `4`.

## Limitações declaradas

O projeto não implementa toda a linguagem Java: não faz compilação real, resolução de imports/classes externas, overload, generics complexos, herança, inferência de tipos de declarações, lambda, method reference, `switch` moderno, exceções, arrays multidimensionais ou inicializados com `{...}` no núcleo semântico. Também não implementa a resolução completa de tipos nominais e construtores, visibilidade, distinção de `static`/instância, inicialização definitiva, caminhos de retorno, contexto de `break/continue` e Unicode completo. Há construções aceitas pelo parser sintático que não estão cobertas pela AST/semântica; nesses casos o programa poderá informar erro de subconjunto. Conversões numéricas e atribuições compostas são aproximadas. A regra didática de escopos permite alguns sombreamentos locais que Java rejeita. O suporte à API externa é restrito a `System.out.print/println`; outras chamadas de membros externos são rejeitadas como fora do subconjunto.

Para evitar estouro de pilha, o parser limita a 128 regras recursivas simultâneas; a AST também tem limite de 128 níveis. Isso não equivale a aceitar 128 pares de parênteses, pois cada construção envolve várias regras. Entradas que excedem os limites recebem diagnóstico, e a árvore é liberada sem recursão.

Para a apresentação use **os arquivos de teste incluídos**, que demonstram o subconjunto efetivamente implementado. Testes desconhecidos da professora podem usar recursos fora do escopo; nesse caso explique a limitação, sem afirmar conformidade integral com Java.

## Testes e refatoração

`make test` executa os seis casos de demonstração, os 70 arquivos em `testes/revisao/` e nove casos temporários de robustez/entrada, totalizando **85 casos**. O script verifica códigos de retorno e mensagens. Os casos temporários cobrem limites de aninhamento, AST profunda, UTF-8 com BOM/CRLF e posição de comentário não fechado. Para executar apenas os arquivos de revisão: `python testes/testar_revisao.py` (ou `python3` em sistemas que usam esse nome).

A refatoração foi orientada por `documentacao/Enunciado_Oficial.pdf`. Manteve os módulos na raiz, sem bibliotecas externas nem novas funcionalidades de Java. Corrigiu as chamadas de diagnóstico que impediam a compilação e separou a construção da AST da análise semântica.

A revisão crítica posterior corrigiu bugs de tipos de elementos/tamanhos de vetores, escopos de classes, promoção numérica, valores `void`, membros e limites de recursão. Os 52 casos da entrega anterior mantiveram a saída completa e o retorno idênticos. Os 24 novos arquivos tiveram suas expectativas confirmadas também com `javac`, usado somente na validação, não pelo compilador.

Neste ambiente, os 85 casos foram executados por PowerShell, com timeout por caso, pois Python não está instalado. Também foram executadas 150 entradas malformadas determinísticas, sem travamento ou retorno inesperado. Os scripts Python e novos sanitizadores não foram executados aqui. A revisão detalhada está em `documentacao/REVISAO_CRITICA.md`; `documentacao/REFATORACAO.md` e `REVISAO_TECNICA.md` registram etapas anteriores.

**Aprovação nos testes não equivale à implementação completa de Java.** Permanecem as limitações do subconjunto descritas acima.

## Materiais de entrega

- Relatório atual: `documentacao/Relatorio_Tecnico_Atualizado.pdf` — 8 páginas, com EBNF, arquitetura, dificuldades, limites, resultados e declaração de apoio de IA.
- Fonte editável do relatório: `documentacao/Relatorio_Tecnico_Atualizado.html`; pode ser impressa em PDF pelo navegador, sem bibliotecas adicionais. O PDF antigo da raiz foi preservado como histórico.
- Apresentação equivalente a slides: `documentacao/APRESENTACAO.md`, com roteiro de 10 minutos e comandos de demonstração.

## Demonstração sugerida (10 minutos)

1. Mostrar estrutura e gramática no relatório (1 min).
2. `./compilador testes/Valido.java` — tokens, tabela de símbolos, AST e zero erros (3 min).
3. `./compilador testes/ErroSintatico.java` e `./compilador testes/ErroLexico.java` — falhas com linha/coluna (2 min).
4. `./compilador testes/ErroSemantico.java` — erros de tipos/escopos/assinaturas (2 min).
5. Descrever limitações e estratégia de validação (2 min).

## Responsabilidade acadêmica

O enunciado exige commits individuais de todos os integrantes e declaração do uso de IA generativa no relatório. O repositório já existe no endereço acima; o grupo precisa manter contribuições individuais reais, revisar e compreender o código e apresentar as três análises. Não atribua commits fictícios aos integrantes. Os slides existentes em `Apresentacao_Compiladores_Java.pptx` foram preservados no repositório; o roteiro atualizado em Markdown é um material complementar.

Há uma contradição no enunciado: a página 1 limita o grupo a três alunos, mas a tabela da página 2 lista quatro integrantes em Java. Os quatro nomes foram mantidos no `main.c`; a composição deve ser confirmada com a professora. O histórico individual completo, a participação e o envio da entrega devem ser conferidos pelo grupo.
