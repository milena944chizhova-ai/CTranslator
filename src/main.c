/*
 * main.c — точка входа транслятора подмножества C
 *
 * Вариант 58, код 222253
 * Метод разбора:     расширенное предшествование операторов
 * Промежуточный код: синтаксическое дерево (AST)
 * Тип транслятора:   интерпретатор
 * Язык реализации:   C (C99)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "semantic.h"
#include "interpreter.h"

/* Максимальный размер исходного файла */
#define MAX_SOURCE  (1024 * 1024)

/* ------------------------------------------------------------------ */
/* Чтение файла в буфер                                                */
/* ------------------------------------------------------------------ */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    if (sz > MAX_SOURCE) { fclose(f); return NULL; }

    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/* ------------------------------------------------------------------ */
/* Запись таблицы лексем в файл                                        */
/* ------------------------------------------------------------------ */
static void write_lexer_output(const LexResult *lr)
{
    FILE *f = fopen("lexer_output.txt", "w");
    if (!f) return;

    fprintf(f, "=== ТАБЛИЦА ЛЕКСЕМ ===\n");
    fprintf(f, "%-5s %-16s %-16s %-8s %s\n",
            "№", "Тип", "Значение", "Строка", "Поз.");
    fprintf(f, "%s\n", "-------------------------------------------------------");

    for (int i = 0; i < lr->count; i++) {
        fprintf(f, "%-5d %-16s %-16s %-8d %d\n",
                i + 1,
                token_type_name(lr->tokens[i].type),
                lr->tokens[i].value,
                lr->tokens[i].line,
                lr->tokens[i].col);
    }

    fprintf(f, "\n");
    if (lr->error_count == 0) {
        fprintf(f, "=== Лексических ошибок не обнаружено ===\n");
    } else {
        fprintf(f, "=== Лексических ошибок: %d ===\n", lr->error_count);
        for (int i = 0; i < lr->error_count; i++)
            fprintf(f, "  %s\n", lr->errors[i]);
    }
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* Запись AST в файл                                                   */
/* ------------------------------------------------------------------ */
static void write_ast_output(AstNode *root)
{
    FILE *f = fopen("ast_output.txt", "w");
    if (!f) return;
    fprintf(f, "=== СИНТАКСИЧЕСКОЕ ДЕРЕВО (AST) ===\n\n");
    ast_print(root, f, "", 1);
    fprintf(f, "\n=== Синтаксических ошибок не обнаружено ===\n");
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* Запись таблицы символов в файл                                      */
/* ------------------------------------------------------------------ */
static void write_sem_output(const SemResult *sr)
{
    FILE *f = fopen("symbols_output.txt", "w");
    if (!f) return;
    semantic_print(sr, f);
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    const char *input_file = (argc > 1) ? argv[1] : "input.txt";

    printf("+-------------------------------------------------+\n");
    printf("| Транслятор подмножества C  (вариант 58, к.222253) |\n");
    printf("| Метод разбора: расширенное предшествование       |\n");
    printf("| Промежуточный код: AST   Тип: интерпретатор     |\n");
    printf("| Язык реализации: C (C99)                        |\n");
    printf("+-------------------------------------------------+\n\n");

    /* ---- Чтение исходного файла ---- */
    char *source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Ошибка: файл '%s' не найден или слишком велик.\n",
                input_file);
        fprintf(stderr, "Использование: translator <input.txt>\n");
        return 1;
    }
    printf("Входной файл: %s\n\n", input_file);

    /* ================================================================ */
    /* 1. ЛЕКСИЧЕСКИЙ АНАЛИЗ                                            */
    /* ================================================================ */
    printf("--- 1. Лексический анализ ---\n");

    static LexResult lr;   /* static: не занимает стек */
    lex(source, &lr);
    free(source);

    write_lexer_output(&lr);
    printf("  Всего лексем: %d\n", lr.count);

    if (lr.error_count > 0) {
        fprintf(stderr, "  Лексических ошибок: %d\n", lr.error_count);
        for (int i = 0; i < lr.error_count; i++)
            fprintf(stderr, "    %s\n", lr.errors[i]);
        printf("  Таблица лексем -> lexer_output.txt\n");
        return 2;
    }
    printf("  Лексических ошибок не обнаружено.\n");
    printf("  Таблица лексем -> lexer_output.txt\n\n");

    /* ================================================================ */
    /* 2. СИНТАКСИЧЕСКИЙ АНАЛИЗ (расширенное предшествование)           */
    /* ================================================================ */
    printf("--- 2. Синтаксический анализ (расширенное предшествование) ---\n");

    char parse_err[PARSE_ERR_LEN] = {0};
    AstNode *ast = parse(lr.tokens, lr.count, parse_err);

    if (!ast) {
        fprintf(stderr, "  %s\n", parse_err);
        return 3;
    }

    write_ast_output(ast);
    printf("  Синтаксический анализ завершён успешно.\n");
    printf("  Синтаксическое дерево -> ast_output.txt\n\n");

    /* ================================================================ */
    /* 3. СЕМАНТИЧЕСКИЙ АНАЛИЗ                                          */
    /* ================================================================ */
    printf("--- 3. Семантический анализ ---\n");

    static SemResult sr;
    semantic_analyze(ast, &sr);
    write_sem_output(&sr);

    if (sr.error_count > 0) {
        fprintf(stderr, "  Семантических ошибок: %d\n", sr.error_count);
        for (int i = 0; i < sr.error_count; i++)
            fprintf(stderr, "    %s\n", sr.errors[i]);
        printf("  Таблица символов -> symbols_output.txt\n");
        return 4;
    }
    printf("  Переменных объявлено: %d\n", sr.sym_count);
    printf("  Семантических ошибок не обнаружено.\n");
    printf("  Таблица символов -> symbols_output.txt\n\n");

    /* ================================================================ */
    /* 4. ИНТЕРПРЕТАЦИЯ                                                 */
    /* ================================================================ */
    printf("--- 4. Выполнение программы ---\n\n");

    int ok = interpret(ast);

    if (ok != 0) {
        printf("\n--- Выполнение завершено с ошибкой ---\n");
        return 5;
    }
    printf("\n--- Трансляция и выполнение завершены ---\n");
    return 0;
}
