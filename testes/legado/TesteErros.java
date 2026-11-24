public class TesteErros {
    public static void main(String[] args) {
        int numero = 0189;          // erro 1: digito invalido em octal (linha 3)
        char letra = 'ab';          // erro 2: mais de um caractere (linha 4)
        char vazio = '';            // erro 3: literal de caractere vazio (linha 5)
        String texto = "sem fim;    // erro 4: texto nao fechado (linha 6)
        int valor = 10 # 20;        // erro 5: caractere invalido (linha 7)
        int outro = 5;
        /* comentario de bloco sem fechamento ...
    }
}
