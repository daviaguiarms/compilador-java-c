"""Testes automatizados da etapa de analise lexica. Executar: python testar.py"""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
exe = root / ('compilador.exe' if sys.platform == 'win32' else 'compilador')
cmd = ['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
       'Compilador.c', 'lexer.c', 'symbol_table.c', '-o', str(exe)]
subprocess.run(cmd, cwd=root, check=True)
cases = [
    ('HelloWorld.java', 0, 0),
    ('testes/TesteCompleto.java', 0, 0),
    ('testes/TesteErros.java', 2, 6),
    ('testes/TesteNumerosCorrigidos.java', 0, 0),
    ('testes/TesteNovosErros.java', 2, 6),
]
for filename, returncode, errors in cases:
    result = subprocess.run([str(exe), filename], cwd=root, capture_output=True,
                            text=True, encoding='utf-8', errors='replace')
    expected = f'Erros lexicos registrados        {errors}'
    ok = result.returncode == returncode and expected in result.stdout
    print(f"{'OK' if ok else 'FALHOU'}: {filename} (erros esperados: {errors})")
    if not ok:
        print(result.stdout[-3000:], result.stderr)
        sys.exit(1)
print('5/5 testes aprovados.')
