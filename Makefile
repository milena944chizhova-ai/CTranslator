# Makefile для транслятора подмножества C (вариант 58)
# Компилятор: GCC (MinGW / MSYS2 / Linux)

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -Isrc
TARGET  = translator

SRCS = src/main.c \
       src/token.c \
       src/lexer.c \
       src/ast.c \
       src/parser.c \
       src/semantic.c \
       src/interpreter.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Запуск тестов
test: $(TARGET)
	./$(TARGET) tests/input.txt
	./$(TARGET) tests/test_arith.txt
	./$(TARGET) tests/test_nested_if.txt

clean:
	rm -f $(TARGET) lexer_output.txt ast_output.txt symbols_output.txt

.PHONY: all test clean
