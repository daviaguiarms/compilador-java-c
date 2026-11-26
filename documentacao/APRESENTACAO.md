# Analisador Java em C — roteiro de apresentação

Material equivalente a slides. Duração: 10 minutos, mais 5 de arguição.
O grupo deve dividir a fala entre **todos os integrantes** e ensaiar os comandos.

## 1. Objetivo e limites — 1 minuto

- Reconhecer um subconjunto de Java, com implementação manual em C11.
- Executar análise léxica, sintática e semântica; não gerar bytecode.
- Referência: `Enunciado_Oficial.pdf` e relatório atualizado nesta pasta.

## 2. Como os módulos se conectam — 1 minuto

```text
main → lexer → tokens e tabela hash → parser → AST → semantic
```

- `ast` representa, imprime e libera os nós.
- Tabela léxica conta identificadores; tabela semântica associa tipos e escopos.
- A escolha de descida recursiva facilita relacionar funções à gramática.

## 3. Programa válido ao vivo — 3 minutos

```powershell
mingw32-make
.\compilador.exe testes\Valido.java
```

No Linux, usar `make` e `./compilador testes/Valido.java`.

- Mostrar tokens e posições, tabela de símbolos, AST indentada e resultado.
- Explicar a chamada `dobrar`, a condição booleana e o escopo de `i` no `for`.
- Abrir `parser.c` e `semantic.c` para relacionar um nó às suas verificações.

## 4. Erro léxico ou sintático ao vivo — 2 minutos

```powershell
.\compilador.exe testes\ErroLexico.java
.\compilador.exe testes\ErroSintatico.java
```

- Mostrar linha, coluna e descrição.
- Explicar por que não continuar às próximas etapas quando a entrada está inválida.

## 5. Erro semântico ao vivo — 2 minutos

```powershell
.\compilador.exe testes\ErroSemantico.java
```

- Explicar declaração antes do uso, redeclaração, tipos e argumentos de chamadas.
- Mostrar que o programa pode ter sintaxe correta e semântica incorreta.

## 6. Validação e limitações — 1 minuto

- 85 casos conferidos: 76 arquivos fixos e 9 casos gerados de robustez/entrada.
- 52 resultados anteriores preservados; 24 novos arquivos conferidos com `javac`.
- 150 entradas malformadas adicionais sem travamento observado.
- Python 3 é necessário para `make test`; nesta revisão os casos foram executados
  por PowerShell, pois Python não está instalado no ambiente.
- Não prometer Java completo: há limitações de classes externas, conversões,
  análise de fluxo e recursos avançados. Declarar o apoio de IA e a responsabilidade
  dos integrantes, conforme o relatório.

## Perguntas para ensaiar

1. Qual a diferença entre token, nó da AST e símbolo semântico?
2. Como a precedência altera a árvore de uma expressão?
3. Por que sair de um escopo remove declarações?
4. Onde são verificadas aridade e compatibilidade de argumentos?
5. Por que passar nos testes não demonstra conformidade completa com Java?
