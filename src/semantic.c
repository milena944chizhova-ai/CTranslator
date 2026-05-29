/*
 * semantic.c — семантический анализатор
 *
 * Обходит AST в глубину (DFS) и ведёт таблицу символов.
 */
#include "semantic.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Вспомогательные функции                                             */
/* ------------------------------------------------------------------ */

static void sem_error(SemResult *r, const char *msg)
{
    if (r->error_count >= MAX_SEM_ERRORS) return;
    strncpy(r->errors[r->error_count], msg, SEM_ERR_LEN - 1);
    r->errors[r->error_count][SEM_ERR_LEN - 1] = '\0';
    r->error_count++;
}

/* Найти символ по имени; возвращает указатель или NULL. */
static Symbol *sym_find(SemResult *r, const char *name)
{
    for (int i = 0; i < r->sym_count; i++)
        if (strcmp(r->table[i].name, name) == 0)
            return &r->table[i];
    return NULL;
}

/* Добавить новый символ. */
static Symbol *sym_add(SemResult *r, const char *name)
{
    if (r->sym_count >= MAX_SYMBOLS) return NULL;
    Symbol *s = &r->table[r->sym_count++];
    strncpy(s->name, name, MAX_NAME - 1);
    s->name[MAX_NAME - 1] = '\0';
    s->initialized = 0;
    return s;
}

/* ------------------------------------------------------------------ */
/* Обход AST                                                           */
/* ------------------------------------------------------------------ */

static void visit(AstNode *node, SemResult *r)
{
    if (!node) return;

    switch (node->kind) {

        case NODE_BLOCK:
            for (int i = 0; i < node->u.block.n; i++)
                visit(node->u.block.stmts[i], r);
            break;

        case NODE_DECL: {
            const char *name = node->u.decl.name;
            if (sym_find(r, name)) {
                char msg[SEM_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Семантическая ошибка: повторное объявление переменной '%s'",
                         name);
                sem_error(r, msg);
            } else {
                sym_add(r, name);
            }
            break;
        }

        case NODE_ASSIGN: {
            const char *name = node->u.assign.target;
            Symbol *s = sym_find(r, name);
            if (!s) {
                char msg[SEM_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Семантическая ошибка: переменная '%s' не объявлена", name);
                sem_error(r, msg);
            }
            visit(node->u.assign.value, r);
            if (s) s->initialized = 1;
            break;
        }

        case NODE_IDENT: {
            const char *name = node->u.ident.name;
            Symbol *s = sym_find(r, name);
            if (!s) {
                char msg[SEM_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Семантическая ошибка: переменная '%s' не объявлена", name);
                sem_error(r, msg);
            } else if (!s->initialized) {
                char msg[SEM_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Семантическая ошибка: переменная '%s'"
                         " используется до инициализации", name);
                sem_error(r, msg);
            }
            break;
        }

        case NODE_IF:
            visit(node->u.if_stmt.cond,    r);
            visit(node->u.if_stmt.then_br, r);
            visit(node->u.if_stmt.else_br, r);
            break;

        case NODE_SCANF:
            for (int i = 0; i < node->u.scanf_nd.n; i++) {
                const char *name = node->u.scanf_nd.vars[i];
                Symbol *s = sym_find(r, name);
                if (!s) {
                    char msg[SEM_ERR_LEN];
                    snprintf(msg, sizeof(msg),
                             "Семантическая ошибка: переменная '%s' не объявлена",
                             name);
                    sem_error(r, msg);
                } else {
                    s->initialized = 1;
                }
            }
            break;

        case NODE_PRINTF:
            for (int i = 0; i < node->u.printf_nd.n; i++)
                visit(node->u.printf_nd.exprs[i], r);
            break;

        case NODE_BINOP:
            visit(node->u.binop.left,  r);
            visit(node->u.binop.right, r);
            break;

        case NODE_UNOP:
            visit(node->u.unop.operand, r);
            break;

        case NODE_NUMBER:
            break;  /* константы не проверяются */
    }
}

/* ------------------------------------------------------------------ */
/* Публичные функции                                                   */
/* ------------------------------------------------------------------ */

void semantic_analyze(AstNode *root, SemResult *r)
{
    r->sym_count   = 0;
    r->error_count = 0;
    visit(root, r);
}

void semantic_print(const SemResult *r, FILE *f)
{
    fprintf(f, "=== ТАБЛИЦА СИМВОЛОВ ===\n");
    fprintf(f, "%-14s%-10s%-20s\n", "Имя", "Тип", "Инициализирована");
    fprintf(f, "%s\n", "----------------------------------------------");
    for (int i = 0; i < r->sym_count; i++) {
        fprintf(f, "%-14s%-10s%-20s\n",
                r->table[i].name, "int",
                r->table[i].initialized ? "да" : "нет");
    }
    fprintf(f, "\n");
    if (r->error_count == 0) {
        fprintf(f, "=== Семантических ошибок не обнаружено ===\n");
    } else {
        fprintf(f, "=== Обнаружено семантических ошибок: %d ===\n",
                r->error_count);
        for (int i = 0; i < r->error_count; i++)
            fprintf(f, "  %s\n", r->errors[i]);
    }
}
