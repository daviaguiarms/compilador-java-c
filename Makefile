CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SRC = main.c lexer.c parser.c semantic.c ast.c symbol_table.c
.PHONY: all clean test
all: compilador
compilador: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@
test: compilador
	python3 testes/rodar_testes.py
	python3 testes/testar_revisao.py
clean:
	rm -f compilador compilador.exe
