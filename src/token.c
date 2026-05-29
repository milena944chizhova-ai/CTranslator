/*
 * token.c — реализация вспомогательных функций для токенов
 */
#include "token.h"

const char *token_type_name(TokenType t)
{
    switch (t) {
        case KW_INT:      return "KW_INT";
        case KW_IF:       return "KW_IF";
        case KW_ELSE:     return "KW_ELSE";
        case KW_SCANF:    return "KW_SCANF";
        case KW_PRINTF:   return "KW_PRINTF";
        case KW_RETURN:   return "KW_RETURN";
        case TK_IDENT:    return "IDENTIFIER";
        case TK_NUMBER:   return "NUMBER";
        case TK_STRING:   return "STRING";
        case OP_PLUS:     return "OP_PLUS";
        case OP_MINUS:    return "OP_MINUS";
        case OP_STAR:     return "OP_STAR";
        case OP_SLASH:    return "OP_SLASH";
        case OP_EQ:       return "OP_EQ";
        case OP_NEQ:      return "OP_NEQ";
        case OP_LT:       return "OP_LT";
        case OP_GT:       return "OP_GT";
        case OP_LE:       return "OP_LE";
        case OP_GE:       return "OP_GE";
        case OP_ASSIGN:   return "OP_ASSIGN";
        case TK_LPAREN:   return "LPAREN";
        case TK_RPAREN:   return "RPAREN";
        case TK_LBRACE:   return "LBRACE";
        case TK_RBRACE:   return "RBRACE";
        case TK_SEMICOLON:return "SEMICOLON";
        case TK_COMMA:    return "COMMA";
        case TK_AMP:      return "AMP";
        case TK_MARKER:   return "MARKER";
        case TK_EOF:      return "EOF";
        default:          return "UNKNOWN";
    }
}
