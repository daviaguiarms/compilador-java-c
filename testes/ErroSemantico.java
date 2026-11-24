public class ErroSemantico {
    static int somar(int x, int y) { return x + y; }
    public static void main(String[] args) {
        int valor = 10;
        int valor = 20;
        int resultado = valor + inexistente;
        boolean ativo = 12;
        int soma = somar(1);
        if (valor) { int local = 2; }
        int falha = local;
    }
}
