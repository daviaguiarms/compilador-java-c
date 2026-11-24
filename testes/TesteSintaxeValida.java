package aula;
public class TesteSintaxeValida {
    private int saldo = 10;
    public static int soma(int a, int b) {
        int resultado = a + b * 2;
        if (resultado > 15) { return resultado; }
        else { return 0; }
    }
    public void rotina() {
        int[] numeros = new int[3];
        numeros[0] = 5;
        for (int i = 0; i < 3; i++) { saldo += i; }
        while (saldo > 0) { saldo--; }
    }
}
