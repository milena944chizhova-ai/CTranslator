/*
 * token.h — типы и структура лексем (токенов)
 * Транслятор подмножества C, вариант 58
 */
#ifndef TOKEN_H
#define TOKEN_H

#define MAX_VALUE 128   /* максимальная длина значения токена */

/* ------------------------------------------------------------------ */
/* Типы лексем                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    /* Ключевые слова */
    KW_INT,    KW_IF,     KW_ELSE,
    KW_SCANF,  KW_PRINTF, KW_RETURN,

    /* Идентификатор, целое число, строковый литерал */
    TK_IDENT, TK_NUMBER, TK_STRING,

    /* Арифметические операторы */
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH,

    /* Операторы сравнения */
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,

    /* Оператор присваивания */
    OP_ASSIGN,

    /* Разделители */
    TK_LPAREN, TK_RPAREN,
    TK_LBRACE, TK_RBRACE,
    TK_SEMICOLON, TK_COMMA, TK_AMP,

    /* Служебные */
    TK_MARKER,  /* маркер дна стека $ */
    TK_EOF
} TokenType;

/* ------------------------------------------------------------------ */
/* Структура токена                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    TokenType type;
    char      value[MAX_VALUE];
    int       line;
    int       col;
} Token;

/* Возвращает текстовое имя типа (для вывода таблицы лексем). */
const char *token_type_name(TokenType t);

#endif /* TOKEN_H */
