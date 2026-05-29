/*
 * ast.c — аллокатор узлов AST и функция печати дерева
 */
#include "ast.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Статический пул узлов                                               */
/* ------------------------------------------------------------------ */
static AstNode node_pool[MAX_NODES];
static int     node_count = 0;

AstNode *ast_alloc(NodeKind kind)
{
    if (node_count >= MAX_NODES) return NULL;
    AstNode *n = &node_pool[node_count++];
    memset(n, 0, sizeof(AstNode));
    n->kind = kind;
    return n;
}

void ast_reset(void)
{
    node_count = 0;
}

/* ------------------------------------------------------------------ */
/* Печать дерева                                                       */
/* ------------------------------------------------------------------ */

static const char *conn(int last)
{
    return last ? "+-- " : "+-- ";
}

/* Строит строку отступа для дочернего уровня. */
static void child_prefix(const char *prefix, int last, char *out, int out_sz)
{
    snprintf(out, out_sz, "%s%s", prefix,
             last ? "    " : "|   ");
}

void ast_print(AstNode *node, FILE *f, const char *prefix, int last)
{
    if (!node) return;
    char cp[512];

    switch (node->kind) {

        case NODE_BLOCK: {
            fprintf(f, "%s%sBlock\n", prefix, conn(last));
            child_prefix(prefix, last, cp, sizeof(cp));
            int n = node->u.block.n;
            for (int i = 0; i < n; i++)
                ast_print(node->u.block.stmts[i], f, cp, i == n - 1);
            break;
        }

        case NODE_DECL:
            fprintf(f, "%s%sDeclaration(int %s)\n",
                    prefix, conn(last), node->u.decl.name);
            break;

        case NODE_ASSIGN: {
            fprintf(f, "%s%sAssign(%s)\n",
                    prefix, conn(last), node->u.assign.target);
            child_prefix(prefix, last, cp, sizeof(cp));
            ast_print(node->u.assign.value, f, cp, 1);
            break;
        }

        case NODE_IF: {
            fprintf(f, "%s%sIf\n", prefix, conn(last));
            child_prefix(prefix, last, cp, sizeof(cp));
            int has_else = (node->u.if_stmt.else_br != NULL);

            /* Condition */
            fprintf(f, "%s+-- Condition:\n", cp);
            char ccp[512];
            snprintf(ccp, sizeof(ccp), "%s|   ", cp);
            ast_print(node->u.if_stmt.cond, f, ccp, 1);

            /* Then */
            fprintf(f, "%s+-- Then:\n", cp);
            char tcp[512];
            snprintf(tcp, sizeof(tcp), "%s%s", cp, has_else ? "|   " : "    ");
            ast_print(node->u.if_stmt.then_br, f, tcp, 1);

            /* Else */
            if (has_else) {
                fprintf(f, "%s+-- Else:\n", cp);
                char ecp[512];
                snprintf(ecp, sizeof(ecp), "%s    ", cp);
                ast_print(node->u.if_stmt.else_br, f, ecp, 1);
            }
            break;
        }

        case NODE_SCANF: {
            fprintf(f, "%s%sScanf([", prefix, conn(last));
            for (int i = 0; i < node->u.scanf_nd.n; i++) {
                if (i) fprintf(f, ", ");
                fprintf(f, "%s", node->u.scanf_nd.vars[i]);
            }
            fprintf(f, "])\n");
            break;
        }

        case NODE_PRINTF: {
            fprintf(f, "%s%sPrintf\n", prefix, conn(last));
            child_prefix(prefix, last, cp, sizeof(cp));
            int n = node->u.printf_nd.n;
            for (int i = 0; i < n; i++)
                ast_print(node->u.printf_nd.exprs[i], f, cp, i == n - 1);
            break;
        }

        case NODE_BINOP: {
            fprintf(f, "%s%sBinaryOp(%s)\n",
                    prefix, conn(last), node->u.binop.op);
            child_prefix(prefix, last, cp, sizeof(cp));
            ast_print(node->u.binop.left,  f, cp, 0);
            ast_print(node->u.binop.right, f, cp, 1);
            break;
        }

        case NODE_UNOP: {
            fprintf(f, "%s%sUnaryOp(%s)\n",
                    prefix, conn(last), node->u.unop.op);
            child_prefix(prefix, last, cp, sizeof(cp));
            ast_print(node->u.unop.operand, f, cp, 1);
            break;
        }

        case NODE_NUMBER:
            fprintf(f, "%s%sNumber(%d)\n",
                    prefix, conn(last), node->u.number.value);
            break;

        case NODE_IDENT:
            fprintf(f, "%s%sIdentifier(%s)\n",
                    prefix, conn(last), node->u.ident.name);
            break;
    }
}
