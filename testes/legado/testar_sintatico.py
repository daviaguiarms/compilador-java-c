"""Compila e testa as fases lexica e sintatica (Python 3 + GCC)."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
exe = root / ('sintatico.exe' if sys.platform == 'win32' else 'sintatico')
subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                'Sintatico.c', 'parser.c', 'lexer.c', 'symbol_table.c', '-o', str(exe)],
               cwd=root, check=True)
casos = [
    ('HelloWorld.java', 0, 'Sintaxe valida'),
    ('testes/TesteCompleto.java', 0, 'Sintaxe valida'),
    ('testes/TesteSintaxeValida.java', 0, 'Sintaxe valida'),
    ('testes/TesteSintaxeInvalida.java', 3, 'ERROS SINTATICOS'),
    ('testes/TesteErros.java', 2, 'ERROS LEXICOS'),
    ('testes/TesteNovosErros.java', 2, 'ERROS LEXICOS'),
]
for arquivo, status, trecho in casos:
    r = subprocess.run([str(exe), arquivo], cwd=root, capture_output=True,
                       text=True, encoding='utf-8', errors='replace')
    correto = r.returncode == status and trecho in r.stdout
    print(f"{'OK' if correto else 'FALHOU'}: {arquivo} (saida={r.returncode})")
    if not correto:
        print(r.stdout, r.stderr)
        sys.exit(1)
print(f'{len(casos)}/{len(casos)} testes sintaticos aprovados.')
