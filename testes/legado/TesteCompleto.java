package exemplo;

import java.util.function.Supplier;

// Comentario de linha: ignorado pelo analisador lexico.
/* Comentario de bloco
   com duas linhas. */

@SuppressWarnings("unused")
public class TesteCompleto {
    public static void main(String[] args) {
        int _contador$1 = 0;
        int numero = 42;
        double preco = 19.90;
        char letra = 'A';
        String texto = "Ola, \"mundo\"!\n";
        boolean ativo = true;
        boolean inativo = false;
        Object nada = null;
        int[] vetor = {1, 2, 3};
        int[] copia = new int[3];

        if (ativo && _contador$1 >= 0) {
            _contador$1++;
        } else {
            _contador$1 += 2;
        }

        for (int i = 0; i < 10; i++) {
            System.out.println(texto + letra + preco);
        }

        while (inativo) {
            break;
        }

        int x = (1 + 2) * 3 % 4;
        x <<= 1;
        x = x >>> 2;
        x >>>= 1;
        x = (x == 5) ? 1 : 0;
        boolean b = !(x != 0) || (x <= 3);
        String resultado = (x >= 0) ? "ok" : "no";

        int mascara = ~0;
        mascara &= 255;
        mascara |= 1;
        mascara ^= 2;
        mascara %= 7;
        mascara >>= 1;

        preco += 0.5;
        preco -= 1.0;
        preco *= 2.0;
        preco /= 2.0;

        Runnable tarefa = () -> System.out.println("lambda");
        tarefa.run();

        Supplier<String> fornecedor = texto::trim;
        System.out.println(fornecedor.get());

        imprimir("a", "b");
        System.out.println(resultado + b + nada + vetor[0] + copia[0] + args.length);
        return;
    }

    private static void imprimir(String... partes) {
        for (String parte : partes) {
            System.out.println(parte);
        }
    }
}
