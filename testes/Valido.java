public class Valido {
    static int dobrar(int a) { return a * 2; }
    public static void main(String[] args) {
        int idade = 18;
        double salario = 3000.5;
        boolean maior = idade >= 18;
        if (maior) { salario = salario + dobrar(2); }
        for (int i = 0; i < 3; i++) { idade += i; }
        System.out.println(salario);
    }
}
