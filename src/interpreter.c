/*
 * interpreter.c — интерпретатор (выполнение по AST)
 *
 * Хранит переменные в таблице mem_table (имя → int).
 * Значение UNINIT (INT_MIN) означает «объявлена, но не инициализирована».
 * При ошибке времени выполнения устанавливается флаг rt_failed.
 */
#include "interpreter.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define UNINIT  INT_MIN
#define MAX_MEM 128    /* максимальное число переменных в программе */

/* ------------------------------------------------------------------ */
/* Таблица переменных (память программы)                               */
/* ------------------------------------------------------------------ */

typedef struct { char name[MAX_NAME]; int value; } MemEntry;

static MemEntry mem_table[MAX_MEM];
static int      mem_count;
static int      rt_failed;

static void rt_error(const char *msg)
{
    if (!rt_failed) {
        fprintf(stderr, "\nОшибка времени выполнения: %s\n", msg);
        rt_failed = 1;
    }
}

static MemEntry *mem_find(const char *name)
{
    for (int i = 0; i < mem_count; i++)
        if (strcmp(mem_table[i].name, name) == 0)
            return &mem_table[i];
    return NULL;
}

static MemEntry *mem_get_or_create(const char *name)
{
    MemEntry *e = mem_find(name);
    if (e) return e;
    if (mem_count >= MAX_MEM) { rt_error("Переполнение таблицы переменных"); return NULL; }
    e = &mem_table[mem_count++];
    strncpy(e->name, name, MAX_NAME - 1);
    e->name[MAX_NAME - 1] = '\0';
    e->value = UNINIT;
    return e;
}

/* ------------------------------------------------------------------ */
/* Вычисление выражения                                                */
/* ------------------------------------------------------------------ */

static int eval(AstNode *node)
{
    if (!node || rt_failed) return 0;

    switch (node->kind) {

        case NODE_NUMBER:
            return node->u.number.value;

        case NODE_IDENT: {
            MemEntry *e = mem_find(node->u.ident.name);
            if (!e || e->value == UNINIT) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "переменная '%s' не инициализирована",
                         node->u.ident.name);
                rt_error(msg);
                return 0;
            }
            return e->value;
        }

        case NODE_UNOP:
            if (node->u.unop.op[0] == '-')
                return -eval(node->u.unop.operand);
            return eval(node->u.unop.operand);

        case NODE_BINOP: {
            const char *op = node->u.binop.op;
            int l = eval(node->u.binop.left);
            if (rt_failed) return 0;
            int r = eval(node->u.binop.right);
            if (rt_failed) return 0;

            if (op[0] == '+' && op[1] == '\0') return l + r;
            if (op[0] == '-' && op[1] == '\0') return l - r;
            if (op[0] == '*') return l * r;
            if (op[0] == '/') {
                if (r == 0) { rt_error("деление на ноль"); return 0; }
                return l / r;
            }
            if (op[0] == '=' && op[1] == '=') return l == r ? 1 : 0;
            if (op[0] == '!' && op[1] == '=') return l != r ? 1 : 0;
            if (op[0] == '<' && op[1] == '\0') return l <  r ? 1 : 0;
            if (op[0] == '<' && op[1] == '=')  return l <= r ? 1 : 0;
            if (op[0] == '>' && op[1] == '\0') return l >  r ? 1 : 0;
            if (op[0] == '>' && op[1] == '=')  return l >= r ? 1 : 0;

            char msg[64];
            snprintf(msg, sizeof(msg), "неизвестный оператор '%s'", op);
            rt_error(msg);
            return 0;
        }

        default:
            rt_error("недопустимый узел в выражении");
            return 0;
    }
}

/* ------------------------------------------------------------------ */
/* Выполнение оператора                                                */
/* ------------------------------------------------------------------ */

static void exec(AstNode *node)
{
    if (!node || rt_failed) return;

    switch (node->kind) {

        case NODE_BLOCK:
            for (int i = 0; i < node->u.block.n && !rt_failed; i++)
                exec(node->u.block.stmts[i]);
            break;

        case NODE_DECL:
            mem_get_or_create(node->u.decl.name);
            break;

        case NODE_ASSIGN: {
            int val = eval(node->u.assign.value);
            if (rt_failed) break;
            MemEntry *e = mem_get_or_create(node->u.assign.target);
            if (e) e->value = val;
            break;
        }

        case NODE_IF:
            if (eval(node->u.if_stmt.cond) != 0)
                exec(node->u.if_stmt.then_br);
            else if (node->u.if_stmt.else_br)
                exec(node->u.if_stmt.else_br);
            break;

        case NODE_SCANF:
            for (int i = 0; i < node->u.scanf_nd.n && !rt_failed; i++) {
                printf("Введите значение для %s: ", node->u.scanf_nd.vars[i]);
                int val;
                if (scanf("%d", &val) != 1) {
                    rt_error("некорректный ввод");
                    break;
                }
                MemEntry *e = mem_get_or_create(node->u.scanf_nd.vars[i]);
                if (e) e->value = val;
            }
            break;

        case NODE_PRINTF:
            for (int i = 0; i < node->u.printf_nd.n && !rt_failed; i++) {
                int val = eval(node->u.printf_nd.exprs[i]);
                if (!rt_failed) printf("%d\n", val);
            }
            break;

        default:
            rt_error("неизвестный тип оператора");
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Вывод состояния памяти                                              */
/* ------------------------------------------------------------------ */

static void print_memory(void)
{
    printf("\n=== Состояние памяти ===\n");
    for (int i = 0; i < mem_count; i++) {
        if (mem_table[i].value == UNINIT)
            printf("  %-10s = <не инициализировано>\n", mem_table[i].name);
        else
            printf("  %-10s = %d\n", mem_table[i].name, mem_table[i].value);
    }
}

/* ------------------------------------------------------------------ */
/* Точка входа                                                         */
/* ------------------------------------------------------------------ */

int interpret(AstNode *root)
{
    mem_count = 0;
    rt_failed = 0;

    exec(root);
    print_memory();

    return rt_failed ? -1 : 0;
}
