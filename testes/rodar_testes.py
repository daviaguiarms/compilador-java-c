import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CASES = [
    ('HelloWorld.java', 0, 'APROVADO NAS TRES ANALISES'),
    ('testes/TesteSintaxeValida.java', 0, 'APROVADO NAS TRES ANALISES'),
    ('testes/Valido.java', 0, 'APROVADO NAS TRES ANALISES'),
    ('testes/ErroLexico.java', 2, 'ERRO LEXICO'),
    ('testes/ErroSintatico.java', 3, 'ERRO SINTATICO'),
    ('testes/ErroSemantico.java', 4, 'ERRO SEMANTICO'),
]
passed = 0
for file, code, expected in CASES:
    p = subprocess.run([str(ROOT / 'compilador'), str(ROOT / file)], capture_output=True, text=True, timeout=10)
    ok = p.returncode == code and expected in p.stdout
    print(f"{'OK' if ok else 'FALHOU'} | {file} | retorno={p.returncode}, esperado={code}")
    if not ok:
        print(p.stdout[-1800:])
        print(p.stderr)
    passed += ok
print(f'RESULTADO: {passed}/{len(CASES)}')
raise SystemExit(0 if passed == len(CASES) else 1)
