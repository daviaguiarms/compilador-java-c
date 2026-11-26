import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXECUTAVEL = ROOT / ("compilador.exe" if os.name == "nt" else "compilador")

CASOS_DEMONSTRACAO = [
    ("HelloWorld.java", 0, "APROVADO NAS TRES ANALISES"),
    ("testes/TesteSintaxeValida.java", 0, "APROVADO NAS TRES ANALISES"),
    ("testes/Valido.java", 0, "APROVADO NAS TRES ANALISES"),
    ("testes/ErroLexico.java", 2, "ERRO LEXICO"),
    ("testes/ErroSintatico.java", 3, "ERRO SINTATICO"),
    ("testes/ErroSemantico.java", 4, "ERRO SEMANTICO"),
]

MENSAGENS_RESULTADO = {
    0: "APROVADO NAS TRES ANALISES",
    2: "REPROVADO: ERRO LEXICO",
    3: "REPROVADO: ERRO SINTATICO",
    4: "REPROVADO: ERRO SEMANTICO",
}


def casos_revisao():
    casos = []
    for arquivo in sorted((ROOT / "testes" / "revisao").glob("*.java")):
        if arquivo.stem.startswith("valid_"):
            retorno = 0
        else:
            retorno = {"invalid_lex": 2, "invalid_syntax": 3}.get(arquivo.stem, 4)
        casos.append((arquivo, retorno, MENSAGENS_RESULTADO[retorno]))
    return casos


def casos_robustez(pasta):
    entradas = [
        ("parenteses.java", "class T { int x = " + "(" * 500 + "1" + ")" * 500 + "; }",
         3, "Limite de aninhamento"),
        ("unarios.java", "class T { boolean x = " + "!" * 500 + "true; }",
         3, "Limite de aninhamento"),
        ("blocos.java", "class T { void f() {" + "{" * 500 + "}" * 500 + "} }",
         3, "Limite de aninhamento"),
        ("atribuicoes.java", "class T { void f() { int x; " + "x=" * 500 + "1; } }",
         3, "Limite de aninhamento"),
        ("inicializadores.java", "class T { int[] x = " + "{" * 500 + "1" + "}" * 500 + "; }",
         3, "Limite de aninhamento"),
        ("soma_longa.java", "class T { int x = " + "+".join(["1"] * 500) + "; }",
         4, "Limite de profundidade da AST"),
        ("membros.java", "class T { void f() { x" + ".x" * 500 + "; } }",
         4, "Limite de profundidade da AST"),
        ("utf8_bom.java", "\ufeffclass T {\r\n// comentario\r\nString texto = \"ação\";\r\n}",
         0, "APROVADO NAS TRES ANALISES"),
        ("comentario_aberto.java", "class T {}\n\n/* comentario",
         2, "Linha 3, coluna 1: comentario de bloco nao fechado"),
    ]
    casos = []
    for nome, fonte, retorno, mensagem in entradas:
        arquivo = pasta / nome
        arquivo.write_bytes(fonte.encode("utf-8"))
        casos.append((arquivo, retorno, mensagem))
    return casos


def executar_casos(casos):
    aprovados = 0
    for arquivo, retorno, mensagem in casos:
        try:
            resultado = subprocess.run(
                [str(EXECUTAVEL), str(ROOT / arquivo)],
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=10,
            )
        except (OSError, subprocess.TimeoutExpired) as erro:
            print(f"FALHOU | {arquivo} | {erro}")
            continue

        passou = resultado.returncode == retorno and mensagem in resultado.stdout
        estado = "OK" if passou else "FALHOU"
        print(f"{estado} | {Path(arquivo).name} | retorno={resultado.returncode}, esperado={retorno}")
        if not passou:
            print(resultado.stdout[-1800:])
            print(resultado.stderr)
        aprovados += passou

    print(f"RESULTADO: {aprovados}/{len(casos)}")
    return aprovados == len(casos)


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="compilador_testes_") as pasta:
        casos = CASOS_DEMONSTRACAO + casos_revisao() + casos_robustez(Path(pasta))
        raise SystemExit(0 if executar_casos(casos) else 1)
