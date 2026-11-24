# Analisador Java (subconjunto) implementado em C

Disciplina: Compiladores — Professora Amanda Danielle Lima de Oliveira

Grupo de Java (apresentação 26/11/2026):
- Bruno Trindade Friaça Moreira
- Davi Aguiar Macedo Santos
- Gustavo Costa Nascimento
- Lucas Henrique de Souza e Sá

## Objetivo

Implementar manualmente, em C11, as análises léxica, sintática e semântica para um **subconjunto didático da linguagem Java**. Não gera bytecode e não substitui `javac`.

## Compilar e executar

É necessário GCC/Clang, `make` e Python 3 (Python apenas para rodar testes). No terminal, a partir desta pasta:

```bash
make
./compilador testes/Valido.java
./compilador testes/ErroLexico.java
./compilador testes/ErroSintatico.java
./compilador testes/ErroSemantico.java
make test
```

No Windows, se `make` não estiver instalado:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c lexer.c parser.c semantic.c ast.c symbol_table.c -o compilador.exe
compilador.exe testes\Valido.java
```

Retorno: `0` válido; `1` erro de uso/arquivo; `2` erro léxico; `3` erro sintático; `4` erro semântico.

## Organização

- `main.c`: driver principal; nomes dos participantes; coleta de tokens; impressão de tabelas, AST e mensagens.
- `lexer.c/.h`: scanner manual sem Lex/Flex, posição dos tokens e erros léxicos.
- `symbol_table.c/.h`: tabela inicial de identificadores lexicais.
- `parser.c/.h`: parser descendente recursivo para validar a sintaxe.
- `ast.c/.h`: representação e impressão indentada da árvore sintática abstrata.
- `semantic.c/.h`: construção da AST para o subconjunto e validações semânticas com escopos.
- `testes/`: entradas válidas e inválidas e script de regressão.
- `Makefile`: build e testes.

## Recursos reconhecidos na fase semântica

Classes simples, campos, métodos estáticos ou de instância, parâmetros, tipos primitivos, `String`, variáveis locais, arrays básicos, chamadas de funções do próprio arquivo, `System.out.println`, expressões aritméticas/relacionais/lógicas, `if/else`, `while`, `for`, `return` e blocos.

As verificações incluem uso de variável não declarada, redeclaração no mesmo escopo, incompatibilidade de tipos, retorno, condições booleanas, escopos aninhados e aridade/tipo de parâmetros de métodos do arquivo. A tabela léxica e a tabela semântica de escopos são diferentes: a primeira registra ocorrências e a segunda associa declarações a tipos e escopos.

## Limitações declaradas

O projeto não implementa toda a linguagem Java: não faz compilação real, resolução de imports/classes externas, overload, generics complexos, herança, inferência de tipos, lambda, method reference, `switch` moderno, exceções, arrays inicializados com `{...}`, controle completo de visibilidade e alguns formatos avançados de literais. Há construções aceitas pelo parser sintático que não estão cobertas pela AST/semântica; nesses casos o programa poderá informar erro de subconjunto. A conversão implícita de tipos numéricos é aproximada. O suporte à API externa é restrito ao exemplo de saída `System.out.println`.

Para a apresentação use **os arquivos de teste incluídos**, que demonstram o subconjunto efetivamente implementado. Testes desconhecidos da professora podem usar recursos fora do escopo; nesse caso explique a limitação, sem afirmar conformidade integral com Java.

## Code review e validação adicional — 24/11/2026

Após a revisão, execute `make clean && make && make test`. A bateria contém os seis casos de demonstração e **40 casos adicionais de regressão** (26 entradas inválidas e 14 entradas válidas). Todos passaram após as correções. Também foram executadas 150 entradas aleatórias, sem travamento ou retorno inesperado, e seis execuções com verificadores AddressSanitizer/UndefinedBehaviorSanitizer, sem alertas observados.

Correções introduzidas: rejeição de variáveis, campos e parâmetros do tipo `void`; validação de incrementos/decrementos numéricos e de seus destinos; verificação de índice inteiro e de acesso apenas a vetores; validação de atribuições compostas; rejeição de nomes de métodos utilizados como variáveis; e verificação básica de aridade e nome de `System.out.print/println`. O driver inclui os quatro integrantes autorizados.

**Aprovação nos testes não equivale à implementação completa de Java.** A implementação semântica cobre apenas o subconjunto documentado. O parser sintático aceita algumas estruturas mais amplas que a AST semântica não representa. Também não são feitas análises completas de classes externas, sobrecarga, generics, conversões de tipos, completude de retornos ou inicialização definitiva de variáveis. Não se deve prometer aceitação de arquivos desconhecidos que utilizem essas construções.

Para detalhes, consulte `REVISAO_TECNICA.md`.

## Demonstração sugerida (10 minutos)

1. Mostrar estrutura e gramática no relatório (1 min).
2. `./compilador testes/Valido.java` — tokens, tabela de símbolos, AST e zero erros (3 min).
3. `./compilador testes/ErroSintatico.java` e `./compilador testes/ErroLexico.java` — falhas com linha/coluna (2 min).
4. `./compilador testes/ErroSemantico.java` — erros de tipos/escopos/assinaturas (2 min).
5. Descrever limitações e estratégia de validação (2 min).

## Responsabilidade acadêmica

O enunciado exige commits individuais de todos os integrantes e declaração do uso de IA generativa no relatório. O grupo precisa criar o repositório e realizar contribuições reais, além de revisar e compreender o código e apresentar as três análises. Não atribua commits fictícios aos integrantes.
