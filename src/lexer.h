/*
 * lexer.h — интерфейс лексического анализатора
 *
 * Лексер реализован в виде конечного автомата (ДКА) с четырьмя состояниями:
 *   S0 — начальное: пропуск пробелов и комментариев
 *   S1 — чтение идентификатора или ключевого слова (буква / '_')
 *   S2 — чтение целочисленной константы (цифра)
 *   S3 — чтение оператора, разделителя или строкового литерала
 */
#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#define MAX_TOKENS    2048
#define MAX_LEX_ERRORS  64
#define LEX_ERR_LEN    256

/* Результат лексического анализа */
typedef struct {
    Token tokens[MAX_TOKENS];
    int   count;

    char  errors[MAX_LEX_ERRORS][LEX_ERR_LEN];
    int   error_count;
} LexResult;

/*
 * lex() — выполняет разбиение исходного текста на лексемы.
 * Все найденные токены записываются в result->tokens[].
 * Лексические ошибки — в result->errors[].
 * Последний токен всегда TK_EOF.
 */
void lex(const char *source, LexResult *result);

#endif /* LEXER_H */
