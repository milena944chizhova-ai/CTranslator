using System;
using System.Collections.Generic;
using System.Text;

namespace CTranslator
{
    public class LexerException : Exception
    {
        public int Line   { get; }
        public int Column { get; }
        public LexerException(string msg, int line, int col)
            : base(msg) { Line = line; Column = col; }
    }

    /// <summary>
    /// Лексический анализатор.
    /// Реализован на основе конечного автомата.
    /// Состояния: S0 (начало), S1 (идентификатор/КС), S2 (число), S3 (оператор/разделитель).
    /// </summary>
    public class Lexer
    {
        private readonly string _src;
        private int _pos, _line, _col;
        private readonly List<string> _errors = new List<string>();

        public Lexer(string source)
        {
            _src = source; _pos = 0; _line = 1; _col = 1;
        }

        /// <summary>Возвращает все токены и список лексических ошибок.</summary>
        public (List<Token> tokens, List<string> errors) Tokenize()
        {
            var tokens = new List<Token>();
            while (true)
            {
                try
                {
                    var tok = NextToken();
                    tokens.Add(tok);
                    if (tok.Type == TokenType.EOF) break;
                }
                catch (LexerException ex)
                {
                    _errors.Add(ex.Message);
                    _pos++; _col++;   // пропустить неизвестный символ
                }
            }
            return (tokens, _errors);
        }

        // ---------- вспомогательные методы ----------

        private void SkipWhitespace()
        {
            while (_pos < _src.Length && char.IsWhiteSpace(_src[_pos]))
            {
                if (_src[_pos] == '\n') { _line++; _col = 1; }
                else _col++;
                _pos++;
            }
        }

        private Token ReadStringLiteral()
        {
            int startLine = _line, startCol = _col;
            var sb = new StringBuilder();
            sb.Append('"');
            while (_pos < _src.Length && _src[_pos] != '"')
            {
                if (_src[_pos] == '\\' && _pos + 1 < _src.Length)
                {
                    sb.Append(_src[_pos]); _pos++; _col++;
                }
                sb.Append(_src[_pos]); _pos++; _col++;
            }
            if (_pos < _src.Length) { sb.Append('"'); _pos++; _col++; }
            return new Token(TokenType.STRING_LIT, sb.ToString(), startLine, startCol);
        }

        /// <summary>
        /// Возвращает очередной токен.
        /// Автомат: S0 → пробелы пропускаются;
        ///          S1 → буква/подчёркивание → идентификатор или ключевое слово;
        ///          S2 → цифра → целочисленная константа;
        ///          S3 → прочее → оператор / разделитель / строковый литерал.
        /// </summary>
        private Token NextToken()
        {
            SkipWhitespace(); // S0

            if (_pos >= _src.Length)
                return new Token(TokenType.EOF, "", _line, _col);

            int tokLine = _line, tokCol = _col;
            char c = _src[_pos];

            // S1: идентификатор или ключевое слово
            if (char.IsLetter(c) || c == '_')
            {
                int start = _pos;
                while (_pos < _src.Length &&
                       (char.IsLetterOrDigit(_src[_pos]) || _src[_pos] == '_'))
                { _pos++; _col++; }

                string word = _src.Substring(start, _pos - start);
                TokenType kw = word switch
                {
                    "int"    => TokenType.KW_INT,
                    "if"     => TokenType.KW_IF,
                    "else"   => TokenType.KW_ELSE,
                    "scanf"  => TokenType.KW_SCANF,
                    "printf" => TokenType.KW_PRINTF,
                    "return" => TokenType.KW_RETURN,
                    _        => TokenType.IDENTIFIER
                };
                return new Token(kw, word, tokLine, tokCol);
            }

            // S2: целочисленная константа
            if (char.IsDigit(c))
            {
                int start = _pos;
                while (_pos < _src.Length && char.IsDigit(_src[_pos]))
                { _pos++; _col++; }
                return new Token(TokenType.NUMBER,
                    _src.Substring(start, _pos - start), tokLine, tokCol);
            }

            // S3: операторы и разделители
            _pos++; _col++;

            // Проверка двухсимвольных операторов
            if (_pos < _src.Length)
            {
                string two = "" + c + _src[_pos];
                switch (two)
                {
                    case "==": _pos++; _col++;
                        return new Token(TokenType.OP_EQ,  "==", tokLine, tokCol);
                    case "!=": _pos++; _col++;
                        return new Token(TokenType.OP_NEQ, "!=", tokLine, tokCol);
                    case "<=": _pos++; _col++;
                        return new Token(TokenType.OP_LE,  "<=", tokLine, tokCol);
                    case ">=": _pos++; _col++;
                        return new Token(TokenType.OP_GE,  ">=", tokLine, tokCol);
                }
            }

            return c switch
            {
                '+' => new Token(TokenType.OP_PLUS,   "+", tokLine, tokCol),
                '-' => new Token(TokenType.OP_MINUS,  "-", tokLine, tokCol),
                '*' => new Token(TokenType.OP_STAR,   "*", tokLine, tokCol),
                '/' => new Token(TokenType.OP_SLASH,  "/", tokLine, tokCol),
                '=' => new Token(TokenType.OP_ASSIGN, "=", tokLine, tokCol),
                '<' => new Token(TokenType.OP_LT,     "<", tokLine, tokCol),
                '>' => new Token(TokenType.OP_GT,     ">", tokLine, tokCol),
                '(' => new Token(TokenType.LPAREN,    "(", tokLine, tokCol),
                ')' => new Token(TokenType.RPAREN,    ")", tokLine, tokCol),
                '{' => new Token(TokenType.LBRACE,    "{", tokLine, tokCol),
                '}' => new Token(TokenType.RBRACE,    "}", tokLine, tokCol),
                ';' => new Token(TokenType.SEMICOLON, ";", tokLine, tokCol),
                ',' => new Token(TokenType.COMMA,     ",", tokLine, tokCol),
                '&' => new Token(TokenType.AMP,       "&", tokLine, tokCol),
                '"' => ReadStringLiteral(),
                _   => throw new LexerException(
                    $"Лексическая ошибка: неизвестный символ '{c}'" +
                    $" в строке {tokLine}, позиция {tokCol}", tokLine, tokCol)
            };
        }
    }
}
