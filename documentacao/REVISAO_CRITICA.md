# Revisão crítica — 26/11/2026

## Parecer

Foram encontradas e corrigidas falhas não detectadas na primeira refatoração.
Não cabe declarar conformidade irrestrita com Java nem entrega acadêmica totalmente
concluída. O código foi validado para o subconjunto documentado; a autoria
individual completa e a confirmação da composição do grupo precisam ser
conferidas pelos integrantes.

Na preparação da publicação foi conferido o repositório
https://github.com/daviaguiarms/compilador-java-c. Ele já existia e continha slides
em PowerPoint, que foram preservados. A ausência de `.git` e de slides na cópia
local não significava ausência desses materiais no GitHub. A integração das
correções deve usar apenas o usuário já configurado, sem simular contribuições
dos demais integrantes.

## Verificação ponto a ponto do enunciado

| Item obrigatório | Status e evidência |
| --- | --- |
| C99 ou superior e módulos exigidos | OK: C11; módulos `lexer`, `parser`, `ast`, `semantic` e `main`, além da tabela hash própria. |
| Scanner manual, sem Lex/Flex | OK: reconhecimento direto em `lexer.c`, sem dependências externas. |
| Identificadores, palavras-chave, números, texto, operadores, delimitadores e comentários | OK no subconjunto documentado; posições e comentários cobertos por testes. |
| Erros léxicos com linha e coluna | OK; caso gerado de comentário aberto conferiu `3:1`. |
| Tabela inicial de símbolos | OK: hash próprio com baldes e colisões encadeadas por índices. |
| Parser consumindo os tokens do lexer | OK: descida recursiva; não invoca `javac` nem outra ferramenta para analisar a entrada. |
| Gramática formal e justificativa da abordagem | OK: EBNF do núcleo e justificativa no novo PDF de 8 páginas. A sintaxe tolerada pelo parser é maior que o núcleo da AST. |
| Variáveis, atribuições, expressões, condicionais, laços e chamadas | OK no subconjunto, demonstrados pelos arquivos incluídos. |
| Erros sintáticos claros e tratamento sem travar | OK nos casos executados; limites explícitos para recursão e profundidade. Não se promete recuperação completa. |
| Construção e percurso da AST | OK; construção no parser, tipos/escopos no módulo semântico e impressão indentada. |
| Declaração antes do uso | OK para variáveis locais do subconjunto; verificação também percorre argumentos e tamanhos de vetores. |
| Compatibilidade de tipos | Corrigido: tipos estáveis por acesso a vetor, promoção numérica, `void`, `null`, `new` e `length`. Conversões completas de Java continuam fora do escopo. |
| Redeclaração e blocos aninhados | OK por nível de escopo; classes agora isoladas. Alguns sombreamentos locais diferem de Java e estão declarados como limitação. |
| Aridade e tipos dos argumentos | Corrigido: métodos da classe corrente, inclusive acessos por `this`/nome da classe; membros externos não resolvidos são rejeitados. |
| Erros semânticos com linha e descrição | OK; retorno `4` e lista de diagnósticos por etapa. |
| Makefile funcional | OK: limpeza e recompilação executadas; alterações em cabeçalhos acionam build. |
| Entrada por argumento e saída de símbolos, erros e AST | OK; etapas posteriores não executam após falha léxica/sintática. |
| Nomes no arquivo principal | OK: os quatro nomes da tabela Java estão em `main.c`. |
| Limite de integrantes | A confirmar: página 1 diz três; tabela Java da página 2 lista quatro. Nenhum nome foi removido nem foi inventada uma autorização. |
| README e testes de demonstração | OK; instruções Unix/Windows, dependências, retornos e limites atualizados. |
| Relatório PDF de 5 a 10 páginas | Corrigido: `Relatorio_Tecnico_Atualizado.pdf`, 8 páginas, exportado e relido após a geração. O anterior foi preservado. |
| Apresentação, 10 minutos e participação de todos | Material pronto em `APRESENTACAO.md`; ensaio, divisão de fala e apresentação são ações do grupo. |
| Demonstração ao vivo de válido, léxico/sintático e semântico | Exemplos e comandos prontos; execução ao vivo depende do grupo. |
| Declaração de IA e referências | OK no relatório atualizado, com finalidade e validação efetivamente realizada. |
| GitHub/GitLab e commits individuais reais | Repositório existente confirmado. O grupo deve conferir a participação individual no histórico. A cópia local inicial não era um repositório; a publicação usa um clone separado e não inventa autores. |
| Envio na data exigida | Ação externa do grupo; não realizada por esta revisão. |

## Correções no código

- O retorno de tipo de `N_INDEX` usava um buffer estático compartilhado: avaliar
  um segundo acesso sobrescrevia o tipo do primeiro. Cada nó agora possui seu
  próprio `inferred_type`, liberado por `ast_free`.
- `N_NEW` não percorria seus filhos: identificadores não declarados e tamanhos
  booleanos passavam. Os filhos agora são verificados; criação com `void`,
  construtor primitivo e vetor sem tamanho também recebem diagnóstico.
- Os membros de uma classe permaneciam no contexto da seguinte. O escopo agora
  é aberto e removido a cada classe.
- Operações com `long` retornavam `int`, e unários em `char` não promoviam para
  `int`. Foram corrigidos esses resultados e a rejeição de usos indevidos de `void`.
- Chamadas por `this`, acesso a campos e `length` podiam ignorar regras de tipos.
  Agora são conferidos, incluindo aridade e proibição de escrita em `length`.
- Recursões de instruções, inicializadores, unários e atribuições não tinham
  limite consistente. Foi adicionado controle comum, verificação de profundidade
  da árvore, impressão limitada e liberação sem recursão.
- A impressão de lexemas passou a usar a rotina existente de escape de controles,
  evitando quebrar a disposição da tabela por caracteres de controle.

## Qualidade e validação executada

- `mingw32-make clean` e build GCC/MinGW com C11, `-Wall -Wextra -Wpedantic -Werror
  -Wshadow -Wstrict-prototypes -O2`: sem avisos ou erros.
- **85/85 casos aprovados:** 6 demonstrações, 70 arquivos de revisão e 9 casos
  gerados de limites/entrada. Retorno e mensagem esperados foram conferidos,
  com timeout de 10 segundos por caso.
- **52/52 saídas completas e retornos anteriores preservados.**
- **24/24 expectativas dos novos arquivos confirmadas com `javac`.** Isso não
  transforma `javac` em dependência nem prova equivalência geral com Java.
- **150/150 entradas malformadas determinísticas** sem travamento, timeout ou
  retorno inesperado observado; timeout de 3 segundos por entrada.
- Python não está instalado: os casos equivalentes foram executados por um
  verificador PowerShell. Os scripts Python ainda precisam ser executados em
  ambiente com Python 3. Não foram executados novos sanitizadores ou Clang.
- Não foram adicionadas bibliotecas ao analisador. O HTML do relatório foi
  exportado pelo navegador em PDF e o resultado foi conferido com 8 páginas.

Os limites, as conversões aproximadas e a diferença entre a sintaxe mais ampla e
o núcleo semântico estão descritos no README e no relatório. Esses avisos são
parte da entrega: não devem ser substituídos por uma alegação de Java completo.
