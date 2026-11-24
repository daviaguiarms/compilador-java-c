from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
files=sorted((root/'testes'/'revisao').glob('*.java'))
ok=0
for f in files:
    expected=0 if f.stem.startswith('valid_') else {'invalid_lex':2,'invalid_syntax':3}.get(f.stem,4)
    p=subprocess.run([str(root/'compilador'),str(f)],capture_output=True,text=True,timeout=10)
    passed=p.returncode==expected
    print(('%s | %-30s | rc=%s esperado=%s' % ('OK' if passed else 'FALHOU',f.stem,p.returncode,expected)))
    if not passed: print(p.stdout[-900:])
    ok+=passed
print('TESTES ADICIONAIS: %d/%d' % (ok,len(files)))
raise SystemExit(0 if ok==len(files) else 1)
