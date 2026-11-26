class T {
    void f() {
        for (int i = 0; i < 3; i++) {
            int dentro = i;
        }
        int fora = dentro;
    }
}
