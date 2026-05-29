/*
 * semantic.h — интерфейс семантического анализатора
 *
 * Проверяемые условия:
 *   - объявление переменной до её использования;
 *   - повторное объявление переменной;
 *   - инициализация переменной до чтения.
 * Ведёт таблицу символов.
 */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <stdio.h>

#define MAX_SYMBOLS     128
#define MAX_SEM_ERRORS   64
#define SEM_ERR_LEN     256

typedef struct {
    char name[MAX_NAME];
    int  initialized;   /* 1 — значение присвоено */
} Symbol;

typedef struct {
    Symbol table[MAX_SYMBOLS];
    int    sym_count;

    char   errors[MAX_SEM_ERRORS][SEM_ERR_LEN];
    int    error_count;
} SemResult;

/* Выполнить семантический анализ AST. */
void semantic_analyze(AstNode *root, SemResult *r);

/* Напечатать таблицу символов и список ошибок в файл. */
void semantic_print(const SemResult *r, FILE *f);

#endif /* SEMANTIC_H */
