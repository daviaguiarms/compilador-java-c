CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SRC = main.c lexer.c parser.c semantic.c ast.c symbol_table.c
HEADERS = lexer.h parser.h semantic.h ast.h symbol_table.h

ifeq ($(OS),Windows_NT)
TARGET = compilador.exe
PYTHON ?= python
CLEAN = cmd /C "if exist compilador.exe del /Q compilador.exe"
else
TARGET = compilador
PYTHON ?= python3
CLEAN = rm -f compilador compilador.exe
endif

.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $@
test: $(TARGET)
	$(PYTHON) testes/rodar_testes.py
clean:
	$(CLEAN)
