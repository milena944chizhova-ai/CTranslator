using System;

namespace CTranslator
{
    public enum TokenType
    {
        // Ключевые слова
        KW_INT, KW_IF, KW_ELSE, KW_SCANF, KW_PRINTF, KW_RETURN,
        // Идентификатор и литералы
        IDENTIFIER, NUMBER, STRING_LIT,
        // Арифметические операторы
        OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH,
        // Операторы сравнения и присваивания
        OP_ASSIGN, OP_EQ, OP_NEQ,
        OP_LT, OP_GT, OP_LE, OP_GE,
        // Разделители
        LPAREN, RPAREN, LBRACE, RBRACE,
        SEMICOLON, COMMA, AMP,
        // Служебные
        MARKER, EOF
    }

    public class Token
    {
        public TokenType Type   { get; }
        public string    Value  { get; }
        public int       Line   { get; }
        public int       Column { get; }

        public Token(TokenType type, string value, int line, int column)
        {
            Type = type; Value = value; Line = line; Column = column;
        }

        public override string ToString() =>
            $"{Type,-15} {Value,-15} L{Line} C{Column}";
    }
}
