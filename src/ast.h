/*
 * ast.h — узлы синтаксического дерева (AST)
 *
 * Узлы реализованы через тегированное объединение (tagged union).
 * Выделение — из статического пула node_pool[].
 */
#ifndef AST_H
#define AST_H

#include <stdio.h>

#define MAX_NAME       64   /* макс. длина имени переменной */
#define MAX_BLOCK     128   /* макс. операторов в блоке     */
#define MAX_ARGS       16   /* макс. аргументов scanf/printf */
#define MAX_NODES    1024   /* размер пула узлов AST         */

/* ------------------------------------------------------------------ */
/* Виды узлов                                                          */
/* ------------------------------------------------------------------ */
typedef enum {
    NODE_BLOCK,     /* составной оператор { ... }           */
    NODE_DECL,      /* объявление: int <name>;               */
    NODE_ASSIGN,    /* присваивание: <target> = <expr>;      */
    NODE_IF,        /* условный оператор                     */
    NODE_SCANF,     /* scanf("%d", &var, ...)                */
    NODE_PRINTF,    /* printf("%d\n", expr, ...)             */
    NODE_BINOP,     /* бинарная операция: left op right      */
    NODE_UNOP,      /* унарная операция: op operand          */
    NODE_NUMBER,    /* целочисленный литерал                 */
    NODE_IDENT      /* идентификатор (имя переменной)        */
} NodeKind;

typedef struct AstNode AstNode;

/* ------------------------------------------------------------------ */
/* Структура узла                                                      */
/* ------------------------------------------------------------------ */
struct AstNode {
    NodeKind kind;
    union {
        /* NODE_BLOCK */
        struct { AstNode *stmts[MAX_BLOCK]; int n; } block;

        /* NODE_DECL */
        struct { char name[MAX_NAME]; } decl;

        /* NODE_ASSIGN */
        struct { char target[MAX_NAME]; AstNode *value; } assign;

        /* NODE_IF */
        struct { AstNode *cond; AstNode *then_br; AstNode *else_br; } if_stmt;

        /* NODE_SCANF */
        struct { char vars[MAX_ARGS][MAX_NAME]; int n; } scanf_nd;

        /* NODE_PRINTF */
        struct { AstNode *exprs[MAX_ARGS]; int n; } printf_nd;

        /* NODE_BINOP */
        struct { char op[4]; AstNode *left; AstNode *right; } binop;

        /* NODE_UNOP */
        struct { char op[4]; AstNode *operand; } unop;

        /* NODE_NUMBER */
        struct { int value; } number;

        /* NODE_IDENT */
        struct { char name[MAX_NAME]; } ident;
    } u;
};

/* ------------------------------------------------------------------ */
/* Функции                                                             */
/* ------------------------------------------------------------------ */

/* Выделить новый узел из пула (возвращает NULL при переполнении). */
AstNode *ast_alloc(NodeKind kind);

/* Сбросить пул (используется между тестами). */
void ast_reset(void);

/* Рекурсивно напечатать поддерево в файл. */
void ast_print(AstNode *node, FILE *f, const char *prefix, int last);

#endif /* AST_H */
