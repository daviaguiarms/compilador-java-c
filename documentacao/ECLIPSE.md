# Compilar e executar no Eclipse CDT / Windows

O projeto usa um Makefile tradicional. O erro `buildConfig is null` ocorre
quando a barra de execução do CDT tenta iniciá-lo com uma configuração de
**Core Build**, que não corresponde a esse projeto. Não é um erro no código C.

## Executar o exemplo válido

1. Feche a mensagem de erro.
2. Selecione o projeto `compilador` e pressione **F5** para atualizar os arquivos.
3. Use **Project > Build Project**. O comando deve ser `mingw32-make all`.
   A mensagem `Nothing to be done for 'all'` indica que o executável já está
   atualizado, não uma falha.
4. Abra **Run > Run Configurations...**.
5. Na categoria **C/C++ Application**, selecione **Compilador_Valido**, a
   configuração compartilhada no arquivo `Compilador_Valido.launch`.
6. Confira **Project:** `compilador` e **C/C++ Application:** `compilador.exe`.
7. Na aba **Arguments**, confira **Program arguments:** `testes/Valido.java` e
   **Working directory:** `${workspace_loc:/compilador}`.
8. Clique em **Run**. A saída deve terminar com `APROVADO NAS TRES ANALISES`.

Não reutilize a execução antiga chamada apenas `compilador`, criada pela barra
Core Build. Depois de iniciar a configuração correta, use **Run History >
Compilador_Valido** para repeti-la. Não é necessário apagar o workspace.

Se a configuração compartilhada não aparecer, feche e reabra o projeto e
pressione F5. Como alternativa, crie uma nova configuração em **C/C++ Application**
com os valores dos passos 6 e 7; não escolha a configuração automática Core Build.

Para outros testes, altere somente **Program arguments**, por exemplo:
`testes/ErroSintatico.java` ou `testes/ErroSemantico.java`. Sem esse argumento,
o programa mostra a mensagem de uso e encerra com retorno `1`.

## Avisos em stdio.h, size_t, NULL e outras definições

Esses avisos podem ser do indexador, mesmo quando o GCC compila corretamente.
Na instalação verificada, o GCC está em `C:\MinGW\bin\gcc.exe`.

1. Abra **Project > Properties > C/C++ General > Paths and Symbols**.
2. Em **Includes**, selecione **GNU C** e adicione os diretórios existentes:
   - `C:\MinGW\include`
   - `C:\MinGW\lib\gcc\mingw32\6.3.0\include`
   - `C:\MinGW\lib\gcc\mingw32\6.3.0\include-fixed`
3. Em **C/C++ General > Preprocessor Include Paths, Macros etc. > Providers**,
   se disponível, habilite o provedor de configurações internas do GCC e confira
   que ele usa o GCC do MinGW. Preserve os demais parâmetros do provedor.
4. Aplique e execute **Project > C/C++ Index > Rebuild**.

Os caminhos acima correspondem a esta instalação, não são uma exigência do
código. Se a versão/local do MinGW mudar, selecione as pastas correspondentes.
Se a página de propriedades não estiver disponível, importe a pasta como
**Makefile Project with Existing Code**, com a toolchain **MinGW GCC**, em um
workspace separado; não exclua arquivos para fazer a importação.

## Conferência independente do Eclipse

No PowerShell aberto na pasta do projeto:

```powershell
mingw32-make
.\compilador.exe testes\Valido.java
```

Esses comandos foram verificados. A configuração `.launch` usa o tipo de execução
tradicional registrado nos plugins do CDT instalado; sua seleção e execução na
interface devem ser conferidas pelo usuário após atualizar o projeto.
