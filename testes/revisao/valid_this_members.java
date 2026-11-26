class T {
    int valor;
    int dobro(int valor) { return valor * 2; }
    void f() {
        boolean valor = true;
        this.valor = this.dobro(2);
        int resultado = this.valor;
    }
}
