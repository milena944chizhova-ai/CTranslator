/*
 * parser.c — синтаксический анализатор
 *
 * ================================================================
 * МЕТОД РАСШИРЕННОГО ПРЕДШЕСТВОВАНИЯ ОПЕРАТОРОВ (Operator Precedence)
 *
 * Грамматика выражений (операторная):
 *   E  ::= E + E  | E - E  | E * E  | E / E
 *   E  ::= E == E | E != E | E < E  | E > E | E <= E | E >= E
 *   E  ::= ( E ) | - E | + E | id | num
 *
 * Приоритеты (от низшего к высшему):
 *   1. Операторы сравнения:  ==  !=  <  >  <=  >=
 *   2. Аддитивные:           +  -
 *   3. Мультипликативные:    *  /
 *   4. Унарный минус:        -E
 *
 * ================================================================
 * ПОЛНАЯ МАТРИЦА ПРЕДШЕСТВОВАНИЯ (14 × 14)
 *
 * Индексы строк/столбцов:
 *   0  = id / num        (TK_IDENT, TK_NUMBER)
 *   1  = +               (OP_PLUS)
 *   2  = -               (OP_MINUS)
 *   3  = *               (OP_STAR)
 *   4  = /               (OP_SLASH)
 *   5  = (               (TK_LPAREN)
 *   6  = )               (TK_RPAREN)
 *   7  = ==              (OP_EQ)
 *   8  = !=              (OP_NEQ)
 *   9  = <  (оп.)        (OP_LT)
 *   10 = >  (оп.)        (OP_GT)
 *   11 = <=              (OP_LE)
 *   12 = >=              (OP_GE)
 *   13 = $  (маркер)     (TK_MARKER / TK_EOF / стоп)
 *
 * Значения:
 *   0 = . (ошибка)
 *   1 = < (сдвиг)
 *   2 = = (равно, сдвиг)
 *   3 = > (свёртка)
 *
 * Обоснование значений:
 *   - id/num всегда «старше» любого последующего оператора → GT(3)
 *     кроме невозможных позиций (.)
 *   - Арифм. бинарные (+, -, *, /) уступают операндам/( → LT(1),
 *     превосходят операторы равного или меньшего приоритета → GT(3)
 *   - Сравнения (==..>=) имеют наименьший приоритет среди операторов:
 *     сдвигают только операнды и ( → LT(1), остальное → GT(3)
 *   - ( сдвигает всё кроме ) и $; ) со скобкой → EQ(2)
 *   - $ сдвигает всё кроме ) и $; со вторым $ → EQ(2)
 * ================================================================
 *
 * Алгоритм (восходящий разбор с предшествованием):
 *   1. Стек содержит терминалы и нетерминалы.
 *   2. Верхний терминал стека сравнивается с текущим входным токеном.
 *   3. LT(<) или EQ(=) → СДВИГ: перенести входной токен в стек.
 *   4. GT(>) → СВЁРТКА:
 *        а) Найти правый конец основы (rightmost terminal).
 *        б) Найти левый конец основы.
 *        в) Применить подходящее правило грамматики.
 *   5. Оба маркера $ → конец разбора.
 *
 * Операторы верхнего уровня (statements) разбираются рекурсивным спуском.
 * ================================================================
 */
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ================================================================ */
/* ПОЛНАЯ МАТРИЦА ПРЕДШЕСТВОВАНИЯ                                   */
/* ================================================================ */

/*      строки: текущий верхний терминал стека
        столбцы: текущий входной токен
        значения: 0=ошибка, 1=LT(сдвиг), 2=EQ(сдвиг), 3=GT(свёртка)
*/
static const int PT[14][14] = {
/*           id   +    -    *    /    (    )   ==   !=    <    >   <=   >=    $  */
/* id/num */ { 0,  3,   3,   3,   3,   0,   3,   3,   3,   3,   3,   3,   3,  3 },
/* +      */ { 1,  3,   3,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* -      */ { 1,  3,   3,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* *      */ { 1,  3,   3,   3,   3,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* /      */ { 1,  3,   3,   3,   3,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* (      */ { 1,  1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,  0 },
/* )      */ { 0,  3,   3,   3,   3,   0,   3,   3,   3,   3,   3,   3,   3,  3 },
/* ==     */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* !=     */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* <      */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* >      */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* <=     */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* >=     */ { 1,  1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,  3 },
/* $      */ { 1,  1,   1,   1,   1,   1,   0,   1,   1,   1,   1,   1,   1,  2 },
};

/* Отображение TokenType → индекс в матрице */
static int token_index(TokenType t)
{
    switch (t) {
        case TK_IDENT:  case TK_NUMBER: return 0;
        case OP_PLUS:   return 1;
        case OP_MINUS:  return 2;
        case OP_STAR:   return 3;
        case OP_SLASH:  return 4;
        case TK_LPAREN: return 5;
        case TK_RPAREN: return 6;
        case OP_EQ:     return 7;
        case OP_NEQ:    return 8;
        case OP_LT:     return 9;
        case OP_GT:     return 10;
        case OP_LE:     return 11;
        case OP_GE:     return 12;
        default:        return 13;  /* $ */
    }
}

/* ================================================================ */
/* Глобальное состояние парсера (не реентерабельно)                 */
/* ================================================================ */

static Token  *g_toks;
static int     g_count;
static int     g_pos;
static char   *g_err;
static int     g_failed;

static void set_error(const char *msg)
{
    if (!g_failed) {
        strncpy(g_err, msg, PARSE_ERR_LEN - 1);
        g_err[PARSE_ERR_LEN - 1] = '\0';
        g_failed = 1;
    }
}

static Token peek(int off)
{
    int i = g_pos + off;
    if (i < 0 || i >= g_count) return g_toks[g_count - 1]; /* EOF */
    return g_toks[i];
}

static Token next_tok(void)
{
    Token t = g_toks[g_pos];
    if (g_pos < g_count - 1) g_pos++;
    return t;
}

static Token expect(TokenType tt)
{
    Token t = peek(0);
    if (t.type != tt) {
        char msg[PARSE_ERR_LEN];
        snprintf(msg, sizeof(msg),
                 "Синтаксическая ошибка: ожидался %s, получен '%s'"
                 " (строка %d, поз. %d)",
                 token_type_name(tt), t.value, t.line, t.col);
        set_error(msg);
    } else {
        next_tok();
    }
    return t;
}

/* ================================================================ */
/* Стек оператора предшествования                                   */
/* ================================================================ */

#define STACK_MAX 256

typedef struct {
    int     is_node;          /* 1 — нетерминал, 0 — терминал */
    TokenType tt;             /* тип терминала (если is_node==0) */
    char    val[MAX_VALUE];   /* значение терминала */
    AstNode *node;            /* нетерминал (если is_node==1) */
} StackItem;

static StackItem stk[STACK_MAX];
static int       stk_top;

static void stk_push_term(TokenType tt, const char *val)
{
    if (stk_top >= STACK_MAX) { set_error("Переполнение стека"); return; }
    stk[stk_top].is_node = 0;
    stk[stk_top].tt      = tt;
    strncpy(stk[stk_top].val, val, MAX_VALUE - 1);
    stk[stk_top].val[MAX_VALUE - 1] = '\0';
    stk[stk_top].node    = NULL;
    stk_top++;
}

static void stk_push_node(AstNode *nd)
{
    if (stk_top >= STACK_MAX) { set_error("Переполнение стека"); return; }
    stk[stk_top].is_node = 1;
    stk[stk_top].tt      = TK_EOF;
    stk[stk_top].val[0]  = '\0';
    stk[stk_top].node    = nd;
    stk_top++;
}

/* Найти индекс верхнего терминала в стеке (-1 если не найден). */
static int top_term_idx(void)
{
    for (int i = stk_top - 1; i >= 0; i--)
        if (!stk[i].is_node) return i;
    return -1;
}

/* ================================================================ */
/* Применение правила грамматики к основе                           */
/* ================================================================ */

static AstNode *apply_rule(StackItem *b, int n)
{
    AstNode *nd;

    /* F → id */
    if (n == 1 && !b[0].is_node && b[0].tt == TK_IDENT) {
        nd = ast_alloc(NODE_IDENT);
        if (!nd) return NULL;
        strncpy(nd->u.ident.name, b[0].val, MAX_NAME - 1);
        return nd;
    }

    /* F → num */
    if (n == 1 && !b[0].is_node && b[0].tt == TK_NUMBER) {
        nd = ast_alloc(NODE_NUMBER);
        if (!nd) return NULL;
        nd->u.number.value = atoi(b[0].val);
        return nd;
    }

    /* Нетерминал без изменений (цепочечное правило: T→F, E→T) */
    if (n == 1 && b[0].is_node)
        return b[0].node;

    /* F → ( E ) */
    if (n == 3
        && !b[0].is_node && b[0].tt == TK_LPAREN
        &&  b[1].is_node
        && !b[2].is_node && b[2].tt == TK_RPAREN)
        return b[1].node;

    /* F → - E  (унарный минус) */
    if (n == 2 && !b[0].is_node && b[0].tt == OP_MINUS && b[1].is_node) {
        nd = ast_alloc(NODE_UNOP);
        if (!nd) return NULL;
        strcpy(nd->u.unop.op, "-");
        nd->u.unop.operand = b[1].node;
        return nd;
    }

    /* F → + E  (унарный плюс — просто возвращаем операнд) */
    if (n == 2 && !b[0].is_node && b[0].tt == OP_PLUS && b[1].is_node)
        return b[1].node;

    /* E → E op E  (бинарная операция) */
    if (n == 3 && b[0].is_node && !b[1].is_node && b[2].is_node) {
        const char *op = NULL;
        switch (b[1].tt) {
            case OP_PLUS:  op = "+";  break;
            case OP_MINUS: op = "-";  break;
            case OP_STAR:  op = "*";  break;
            case OP_SLASH: op = "/";  break;
            case OP_EQ:    op = "=="; break;
            case OP_NEQ:   op = "!="; break;
            case OP_LT:    op = "<";  break;
            case OP_GT:    op = ">";  break;
            case OP_LE:    op = "<="; break;
            case OP_GE:    op = ">="; break;
            default:       break;
        }
        if (op) {
            nd = ast_alloc(NODE_BINOP);
            if (!nd) return NULL;
            strncpy(nd->u.binop.op, op, 3);
            nd->u.binop.op[3]    = '\0';
            nd->u.binop.left     = b[0].node;
            nd->u.binop.right    = b[2].node;
            return nd;
        }
    }

    return NULL;
}

/* ================================================================ */
/* Свёртка стека                                                    */
/* ================================================================ */
static void reduce_stack(void)
{
    /* Шаг 1: найти правый конец основы (крайний правый терминал) */
    int ti = top_term_idx();
    if (ti <= 0) {
        set_error("Ошибка свёртки: основа без терминала");
        return;
    }

    /* Шаг 2: определить левую границу основы */
    int basis_start = ti;

    /* Включить нетерминал слева от topTerm (если есть) */
    if (ti > 0 && stk[ti - 1].is_node)
        basis_start = ti - 1;

    /* Особый случай: ) → найти соответствующую ( */
    if (stk[ti].tt == TK_RPAREN) {
        int depth = 1;
        int k = ti - 1;
        while (k >= 0 && depth > 0) {
            if (!stk[k].is_node) {
                if (stk[k].tt == TK_RPAREN) depth++;
                if (stk[k].tt == TK_LPAREN) depth--;
            }
            k--;
        }
        basis_start = k + 1; /* позиция ( */
    }

    /* Шаг 3: скопировать основу */
    int basis_len = stk_top - basis_start;
    StackItem basis[64];
    if (basis_len > 64) basis_len = 64;
    for (int i = 0; i < basis_len; i++)
        basis[i] = stk[basis_start + i];

    stk_top = basis_start;

    /* Шаг 4: применить правило */
    AstNode *nd = apply_rule(basis, basis_len);
    if (!nd) {
        set_error("Нет подходящего правила для свёртки основы");
        /* вернуть основу обратно для диагностики */
        for (int i = 0; i < basis_len; i++)
            stk[stk_top++] = basis[i];
        return;
    }

    stk_push_node(nd);
}

/* ================================================================ */
/* Проверка: является ли токен арифметическим (входит в выражение)  */
/* ================================================================ */
static int is_arith_token(TokenType tt)
{
    switch (tt) {
        case TK_IDENT: case TK_NUMBER:
        case OP_PLUS:  case OP_MINUS:
        case OP_STAR:  case OP_SLASH:
        case OP_EQ:    case OP_NEQ:
        case OP_LT:    case OP_GT:
        case OP_LE:    case OP_GE:
        case TK_LPAREN: case TK_RPAREN:
            return 1;
        default:
            return 0;
    }
}

/* Является ли токен стоп-токеном (концом текущего выражения)? */
static int is_stop(TokenType tt, int depth, TokenType *stop_at, int stop_n)
{
    if (tt == TK_EOF) return 1;
    for (int i = 0; i < stop_n; i++) {
        if (tt == stop_at[i]) {
            /* ) является стоп-токеном только при depth == 0 */
            if (tt == TK_RPAREN && depth > 0) continue;
            return 1;
        }
    }
    /* Токен, не входящий в выражение, тоже стоп-токен */
    return !is_arith_token(tt);
}

/* ================================================================ */
/* МЕТОД ПРЕДШЕСТВОВАНИЯ: разбор одного выражения                   */
/* ================================================================ */
static AstNode *parse_expr(TokenType *stop_at, int stop_n)
{
    if (g_failed) return NULL;

    /* Инициализация стека: маркер дна $ */
    stk_top = 0;
    stk_push_term(TK_MARKER, "$");

    int depth = 0;  /* счётчик открытых скобок внутри выражения */

    while (!g_failed) {
        /* Верхний терминал стека */
        int ti = top_term_idx();
        if (ti < 0) { set_error("Пустой стек предшествования"); break; }
        TokenType top_tt = stk[ti].tt;

        /* Текущий входной токен */
        Token cur = peek(0);
        int   end = is_stop(cur.type, depth, stop_at, stop_n);
        TokenType cur_tt = end ? TK_MARKER : cur.type;

        int row = token_index(top_tt);
        int col = token_index(cur_tt);
        int rel = PT[row][col];

        /* Оба маркера → конец разбора */
        if (top_tt == TK_MARKER && end) {
            if (stk_top == 2 && stk[1].is_node)
                return stk[1].node;
            if (stk_top == 1) {
                char msg[PARSE_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Ожидалось выражение у '%s' (строка %d)",
                         cur.value, cur.line);
                set_error(msg);
                return NULL;
            }
            /* Финальная свёртка */
            reduce_stack();
            if (!g_failed && stk_top == 2 && stk[1].is_node)
                return stk[1].node;
            if (!g_failed) {
                char msg[PARSE_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Неполное выражение у '%s' (строка %d)",
                         cur.value, cur.line);
                set_error(msg);
            }
            return NULL;
        }

        if (rel == 1 || rel == 2) {
            /* СДВИГ */
            if (cur.type == TK_LPAREN) depth++;
            if (cur.type == TK_RPAREN) depth--;
            stk_push_term(cur.type, cur.value);
            next_tok();
        } else if (rel == 3) {
            /* СВЁРТКА */
            reduce_stack();
        } else {
            /* Ошибка (rel == 0) */
            char msg[PARSE_ERR_LEN];
            snprintf(msg, sizeof(msg),
                     "Синтаксическая ошибка в выражении: неожиданный '%s'"
                     " (строка %d, поз. %d)",
                     cur.value, cur.line, cur.col);
            set_error(msg);
            return NULL;
        }
    }
    return NULL;
}

/* ================================================================ */
/* РЕКУРСИВНЫЙ СПУСК: операторы                                     */
/* ================================================================ */

static AstNode *parse_stmt(void);  /* forward */

static AstNode *parse_block(void)
{
    if (g_failed) return NULL;
    expect(TK_LBRACE);
    AstNode *blk = ast_alloc(NODE_BLOCK);
    if (!blk) { set_error("Нет памяти для блока"); return NULL; }
    while (!g_failed
           && peek(0).type != TK_RBRACE
           && peek(0).type != TK_EOF) {
        AstNode *s = parse_stmt();
        if (s && blk->u.block.n < MAX_BLOCK)
            blk->u.block.stmts[blk->u.block.n++] = s;
    }
    expect(TK_RBRACE);
    return blk;
}

static AstNode *parse_decl(void)
{
    if (g_failed) return NULL;
    expect(KW_INT);
    Token id = expect(TK_IDENT);
    expect(TK_SEMICOLON);
    AstNode *nd = ast_alloc(NODE_DECL);
    if (nd) strncpy(nd->u.decl.name, id.value, MAX_NAME - 1);
    return nd;
}

static AstNode *parse_assign(void)
{
    if (g_failed) return NULL;
    Token id = expect(TK_IDENT);
    expect(OP_ASSIGN);
    TokenType stop[] = { TK_SEMICOLON };
    AstNode *val = parse_expr(stop, 1);
    expect(TK_SEMICOLON);
    AstNode *nd = ast_alloc(NODE_ASSIGN);
    if (nd) {
        strncpy(nd->u.assign.target, id.value, MAX_NAME - 1);
        nd->u.assign.value = val;
    }
    return nd;
}

static AstNode *parse_if(void)
{
    if (g_failed) return NULL;
    expect(KW_IF);
    expect(TK_LPAREN);
    /* Выражение-условие разбирается методом предшествования.
       Стоп-токен: ), но только на глубине 0 (depth отслеживается внутри). */
    TokenType stop[] = { TK_RPAREN };
    AstNode *cond = parse_expr(stop, 1);
    expect(TK_RPAREN);
    AstNode *then_br = parse_stmt();
    AstNode *else_br = NULL;
    if (!g_failed && peek(0).type == KW_ELSE) {
        next_tok();
        else_br = parse_stmt();
    }
    AstNode *nd = ast_alloc(NODE_IF);
    if (nd) {
        nd->u.if_stmt.cond    = cond;
        nd->u.if_stmt.then_br = then_br;
        nd->u.if_stmt.else_br = else_br;
    }
    return nd;
}

static AstNode *parse_scanf(void)
{
    if (g_failed) return NULL;
    expect(KW_SCANF);
    expect(TK_LPAREN);
    expect(TK_STRING);
    AstNode *nd = ast_alloc(NODE_SCANF);
    while (!g_failed && peek(0).type == TK_COMMA) {
        next_tok();
        expect(TK_AMP);
        Token v = expect(TK_IDENT);
        if (nd && nd->u.scanf_nd.n < MAX_ARGS) {
            strncpy(nd->u.scanf_nd.vars[nd->u.scanf_nd.n++],
                    v.value, MAX_NAME - 1);
        }
    }
    expect(TK_RPAREN);
    expect(TK_SEMICOLON);
    return nd;
}

static AstNode *parse_printf(void)
{
    if (g_failed) return NULL;
    expect(KW_PRINTF);
    expect(TK_LPAREN);
    expect(TK_STRING);
    AstNode *nd = ast_alloc(NODE_PRINTF);
    while (!g_failed && peek(0).type == TK_COMMA) {
        next_tok();
        TokenType stop[] = { TK_COMMA, TK_RPAREN };
        AstNode *e = parse_expr(stop, 2);
        if (e && nd && nd->u.printf_nd.n < MAX_ARGS)
            nd->u.printf_nd.exprs[nd->u.printf_nd.n++] = e;
    }
    expect(TK_RPAREN);
    expect(TK_SEMICOLON);
    return nd;
}

static AstNode *parse_return(void)
{
    if (g_failed) return NULL;
    expect(KW_RETURN);
    TokenType stop[] = { TK_SEMICOLON };
    AstNode *e = parse_expr(stop, 1);
    expect(TK_SEMICOLON);
    /* return X; → представляем как printf без формата */
    AstNode *nd = ast_alloc(NODE_PRINTF);
    if (nd && e) nd->u.printf_nd.exprs[nd->u.printf_nd.n++] = e;
    return nd;
}

static AstNode *parse_stmt(void)
{
    if (g_failed) return NULL;
    Token t = peek(0);
    switch (t.type) {
        case KW_INT:    return parse_decl();
        case KW_IF:     return parse_if();
        case TK_LBRACE: return parse_block();
        case KW_SCANF:  return parse_scanf();
        case KW_PRINTF: return parse_printf();
        case KW_RETURN: return parse_return();
        case TK_IDENT:
            if (peek(1).type == OP_ASSIGN)
                return parse_assign();
            /* fall-through */
        default: {
            char msg[PARSE_ERR_LEN];
            snprintf(msg, sizeof(msg),
                     "Синтаксическая ошибка: неожиданный '%s'"
                     " (строка %d, поз. %d)",
                     t.value, t.line, t.col);
            set_error(msg);
            return NULL;
        }
    }
}

/* ================================================================ */
/* Точка входа                                                       */
/* ================================================================ */
AstNode *parse(Token *tokens, int count, char *err)
{
    g_toks   = tokens;
    g_count  = count;
    g_pos    = 0;
    g_err    = err;
    g_failed = 0;
    err[0]   = '\0';

    AstNode *root = ast_alloc(NODE_BLOCK);
    if (!root) {
        strncpy(err, "Нет памяти для корневого блока", PARSE_ERR_LEN - 1);
        return NULL;
    }

    while (!g_failed && peek(0).type != TK_EOF) {
        AstNode *s = parse_stmt();
        if (s && root->u.block.n < MAX_BLOCK)
            root->u.block.stmts[root->u.block.n++] = s;
    }

    return g_failed ? NULL : root;
}
