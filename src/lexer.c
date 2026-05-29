/*
 * lexer.c — лексический анализатор (конечный автомат)
 *
 * Диаграмма переходов ДКА:
 *
 *   +----------+  пробел/\n  +---------+
 *   |          |<------------|         |
 *   |    S0    |  буква/'_'  |   S1    | → идентификатор / ключевое слово
 *   | (начало) |------------>|         |
 *   |          |  цифра      +---------+
 *   |          |------------>|   S2    | → целая константа
 *   |          |  прочее     +---------+
 *   |          |------------>|   S3    | → оператор / разделитель / строка
 *   +----------+             +---------+
 *
 * Из S0 также пропускаются однострочные комментарии (//).
 */
#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Состояния ДКА                                                       */
/* ------------------------------------------------------------------ */
typedef enum { S0, S1, S2, S3 } LexState;

/* ------------------------------------------------------------------ */
/* Вспомогательные функции                                             */
/* ------------------------------------------------------------------ */

static void add_token(LexResult *r, TokenType type,
                      const char *value, int line, int col)
{
    if (r->count >= MAX_TOKENS) return;
    r->tokens[r->count].type = type;
    r->tokens[r->count].line = line;
    r->tokens[r->count].col  = col;
    strncpy(r->tokens[r->count].value, value, MAX_VALUE - 1);
    r->tokens[r->count].value[MAX_VALUE - 1] = '\0';
    r->count++;
}

static void add_error(LexResult *r, const char *msg)
{
    if (r->error_count >= MAX_LEX_ERRORS) return;
    strncpy(r->errors[r->error_count], msg, LEX_ERR_LEN - 1);
    r->errors[r->error_count][LEX_ERR_LEN - 1] = '\0';
    r->error_count++;
}

/* ------------------------------------------------------------------ */
/* Основная функция лексера                                            */
/* ------------------------------------------------------------------ */
void lex(const char *source, LexResult *result)
{
    result->count       = 0;
    result->error_count = 0;

    int pos  = 0;
    int line = 1;
    int col  = 1;
    int len  = (int)strlen(source);

    while (pos < len) {
        char c = source[pos];

        /* ---- S0: пробелы и переносы строк ---- */
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++; col++;
            continue;
        }
        if (c == '\n') {
            pos++; line++; col = 1;
            continue;
        }

        /* ---- S0: однострочный комментарий // ---- */
        if (c == '/' && pos + 1 < len && source[pos + 1] == '/') {
            while (pos < len && source[pos] != '\n')
                pos++;
            continue;
        }

        int tok_line = line;
        int tok_col  = col;

        /* ---- S1: идентификатор или ключевое слово ---- */
        if (isalpha((unsigned char)c) || c == '_') {
            int start = pos;
            while (pos < len &&
                   (isalnum((unsigned char)source[pos]) || source[pos] == '_')) {
                pos++; col++;
            }
            int wlen = pos - start;
            if (wlen >= MAX_VALUE) wlen = MAX_VALUE - 1;

            char word[MAX_VALUE] = {0};
            strncpy(word, source + start, wlen);

            TokenType kw;
            if      (strcmp(word, "int")    == 0) kw = KW_INT;
            else if (strcmp(word, "if")     == 0) kw = KW_IF;
            else if (strcmp(word, "else")   == 0) kw = KW_ELSE;
            else if (strcmp(word, "scanf")  == 0) kw = KW_SCANF;
            else if (strcmp(word, "printf") == 0) kw = KW_PRINTF;
            else if (strcmp(word, "return") == 0) kw = KW_RETURN;
            else                                   kw = TK_IDENT;

            add_token(result, kw, word, tok_line, tok_col);
            continue;
        }

        /* ---- S2: целочисленная константа ---- */
        if (isdigit((unsigned char)c)) {
            int start = pos;
            while (pos < len && isdigit((unsigned char)source[pos])) {
                pos++; col++;
            }
            int nlen = pos - start;
            if (nlen >= MAX_VALUE) nlen = MAX_VALUE - 1;

            char num[MAX_VALUE] = {0};
            strncpy(num, source + start, nlen);
            add_token(result, TK_NUMBER, num, tok_line, tok_col);
            continue;
        }

        /* ---- S3: операторы, разделители, строковый литерал ---- */
        pos++; col++;

        /* Двухсимвольные операторы */
        if (pos < len) {
            char nx = source[pos];
            if (c == '=' && nx == '=') { pos++; col++; add_token(result, OP_EQ,  "==", tok_line, tok_col); continue; }
            if (c == '!' && nx == '=') { pos++; col++; add_token(result, OP_NEQ, "!=", tok_line, tok_col); continue; }
            if (c == '<' && nx == '=') { pos++; col++; add_token(result, OP_LE,  "<=", tok_line, tok_col); continue; }
            if (c == '>' && nx == '=') { pos++; col++; add_token(result, OP_GE,  ">=", tok_line, tok_col); continue; }
        }

        /* Строковый литерал */
        if (c == '"') {
            char str[MAX_VALUE] = {0};
            int si = 0;
            str[si++] = '"';
            while (pos < len && source[pos] != '"' && source[pos] != '\n') {
                if (source[pos] == '\\' && pos + 1 < len) {
                    if (si < MAX_VALUE - 2) str[si++] = source[pos];
                    pos++; col++;
                }
                if (si < MAX_VALUE - 1) str[si++] = source[pos];
                pos++; col++;
            }
            if (pos < len && source[pos] == '"') {
                if (si < MAX_VALUE - 1) str[si++] = '"';
                pos++; col++;
            }
            str[si] = '\0';
            add_token(result, TK_STRING, str, tok_line, tok_col);
            continue;
        }

        /* Односимвольные операторы и разделители */
        switch (c) {
            case '+': add_token(result, OP_PLUS,     "+", tok_line, tok_col); break;
            case '-': add_token(result, OP_MINUS,    "-", tok_line, tok_col); break;
            case '*': add_token(result, OP_STAR,     "*", tok_line, tok_col); break;
            case '/': add_token(result, OP_SLASH,    "/", tok_line, tok_col); break;
            case '=': add_token(result, OP_ASSIGN,   "=", tok_line, tok_col); break;
            case '<': add_token(result, OP_LT,       "<", tok_line, tok_col); break;
            case '>': add_token(result, OP_GT,       ">", tok_line, tok_col); break;
            case '(': add_token(result, TK_LPAREN,   "(", tok_line, tok_col); break;
            case ')': add_token(result, TK_RPAREN,   ")", tok_line, tok_col); break;
            case '{': add_token(result, TK_LBRACE,   "{", tok_line, tok_col); break;
            case '}': add_token(result, TK_RBRACE,   "}", tok_line, tok_col); break;
            case ';': add_token(result, TK_SEMICOLON,";", tok_line, tok_col); break;
            case ',': add_token(result, TK_COMMA,    ",", tok_line, tok_col); break;
            case '&': add_token(result, TK_AMP,      "&", tok_line, tok_col); break;
            default: {
                char msg[LEX_ERR_LEN];
                snprintf(msg, sizeof(msg),
                         "Лексическая ошибка: неизвестный символ '%c'"
                         " (строка %d, поз. %d)",
                         c, tok_line, tok_col);
                add_error(result, msg);
            }
        }
    }

    /* Всегда завершаем токеном EOF */
    add_token(result, TK_EOF, "", line, col);
}
