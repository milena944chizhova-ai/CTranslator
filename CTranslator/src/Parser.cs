using System;
using System.Collections.Generic;

namespace CTranslator
{
    public class SyntaxException : Exception
    {
        public SyntaxException(string msg) : base(msg) { }
    }

    // ===========================================================
    // Синтаксический анализатор — МЕТОД РАСШИРЕННОГО ПРЕДШЕСТВОВАНИЯ
    //
    // Вариант 58, код 222253: метод разбора — цифра 5.
    //
    // Грамматика арифметических выражений:
    //   E ::= E + T | E - T | T
    //   T ::= T * F | T / F | F
    //   F ::= ( E ) | id | num | - F | + F
    //
    // Таблица предшествования (Si строка, Sj столбец):
    //   Индексы: id/num=0, +=1, -=2, *=3, /=4, (=5, )=6, $=7
    //
    //        0    1    2    3    4    5    6    7
    //   0    .    >    >    >    >    .    >    >
    //   1    <    >    >    <    <    <    >    >
    //   2    <    >    >    <    <    <    >    >
    //   3    <    >    >    >    >    <    >    >
    //   4    <    >    >    >    >    <    >    >
    //   5    <    <    <    <    <    <    =    .
    //   6    .    >    >    >    >    .    >    >
    //   7    <    <    <    <    <    <    .    =
    //
    // Классический алгоритм восходящего разбора с предшествованием:
    //   1. На стеке символы разбора (терминалы и нетерминалы).
    //   2. Верхний терминал стека сравнивается с текущим входным.
    //   3. < или = → СДВИГ (перенести токен в стек).
    //   4. > → СВЁРТКА:
    //      а. Найти вершину стека k, такую что prev_terminal(k) > token_at_k.
    //         Это правый конец основы.
    //      б. Найти начало основы: двигаться влево пока prev_term < symbol.
    //      в. Применить правило грамматики.
    //   5. Оба $ → конец.
    //
    // Неоднозначности разрешаются таблицами троек:
    //   LT = { ($ id =), (( id +), (+ num *), (( num +) }
    //   RT = { (id ) $), (num ) $) }
    //
    // Операторы верхнего уровня разбираются рекурсивным спуском.
    // ===========================================================
    public class PrecedenceParser
    {
        readonly List<Token> _tok;
        int _pos;

        const int I_ID  = 0;
        const int I_ADD = 1;
        const int I_SUB = 2;
        const int I_MUL = 3;
        const int I_DIV = 4;
        const int I_LP  = 5;
        const int I_RP  = 6;
        const int I_END = 7;

        // 0=нет, 1=<(LT), 2==(EQ), 3=>(GT)
        static readonly int[,] PT = {
            //   id   +    -    *    /    (    )    $
            {  0,   3,   3,   3,   3,   0,   3,   3 }, // 0 id/num
            {  1,   3,   3,   1,   1,   1,   3,   3 }, // 1 +
            {  1,   3,   3,   1,   1,   1,   3,   3 }, // 2 -
            {  1,   3,   3,   3,   3,   1,   3,   3 }, // 3 *
            {  1,   3,   3,   3,   3,   1,   3,   3 }, // 4 /
            {  1,   1,   1,   1,   1,   1,   2,   0 }, // 5 (
            {  0,   3,   3,   3,   3,   0,   3,   3 }, // 6 )
            {  1,   1,   1,   1,   1,   1,   0,   2 }, // 7 $
        };

        public PrecedenceParser(List<Token> toks) { _tok = toks; _pos = 0; }

        Token Peek(int off = 0)
        {
            int i = _pos + off;
            return i < _tok.Count ? _tok[i] : _tok[_tok.Count - 1];
        }

        Token Next()
        {
            var t = _tok[_pos];
            if (_pos < _tok.Count - 1) _pos++;
            return t;
        }

        Token Expect(TokenType tt)
        {
            var t = Peek();
            if (t.Type != tt)
                throw new SyntaxException(
                    $"Синтаксическая ошибка: ожидался {tt}, получен '{t.Value}' " +
                    $"(строка {t.Line}, поз. {t.Column})");
            return Next();
        }

        // ============================================================
        // Точка входа
        // ============================================================
        public AstNode Parse()
        {
            var blk = new BlockNode();
            while (Peek().Type != TokenType.EOF)
                blk.Statements.Add(Stmt());
            return blk;
        }

        // ============================================================
        // Рекурсивный спуск: операторы
        // ============================================================
        AstNode Stmt()
        {
            var t = Peek();
            switch (t.Type)
            {
                case TokenType.KW_INT:    return Decl();
                case TokenType.KW_IF:     return IfStmt();
                case TokenType.LBRACE:    return Block();
                case TokenType.KW_SCANF:  return ScanfStmt();
                case TokenType.KW_PRINTF: return PrintfStmt();
                case TokenType.KW_RETURN: return RetStmt();
                case TokenType.IDENTIFIER when Peek(1).Type == TokenType.OP_ASSIGN:
                    return AssignStmt();
                default:
                    throw new SyntaxException(
                        $"Синтаксическая ошибка: неожиданный '{t.Value}' " +
                        $"(строка {t.Line}, поз. {t.Column})");
            }
        }

        AstNode Decl()
        {
            Expect(TokenType.KW_INT);
            var n = Expect(TokenType.IDENTIFIER).Value;
            Expect(TokenType.SEMICOLON);
            return new DeclarationNode { Name = n };
        }

        AstNode AssignStmt()
        {
            var n = Expect(TokenType.IDENTIFIER).Value;
            Expect(TokenType.OP_ASSIGN);
            var v = Expr(TokenType.SEMICOLON);
            Expect(TokenType.SEMICOLON);
            return new AssignNode { Target = n, Value = v };
        }

        AstNode IfStmt()
        {
            Expect(TokenType.KW_IF);
            Expect(TokenType.LPAREN);
            var cond = Cond();
            Expect(TokenType.RPAREN);
            var then = Stmt();
            AstNode? els = null;
            if (Peek().Type == TokenType.KW_ELSE) { Next(); els = Stmt(); }
            return new IfNode { Condition = cond, ThenBranch = then, ElseBranch = els };
        }

        AstNode Block()
        {
            Expect(TokenType.LBRACE);
            var blk = new BlockNode();
            while (Peek().Type != TokenType.RBRACE && Peek().Type != TokenType.EOF)
                blk.Statements.Add(Stmt());
            Expect(TokenType.RBRACE);
            return blk;
        }

        AstNode ScanfStmt()
        {
            Expect(TokenType.KW_SCANF);
            Expect(TokenType.LPAREN);
            Expect(TokenType.STRING_LIT);
            var node = new ScanfNode();
            while (Peek().Type == TokenType.COMMA)
            {
                Next();
                Expect(TokenType.AMP);
                node.Variables.Add(Expect(TokenType.IDENTIFIER).Value);
            }
            Expect(TokenType.RPAREN);
            Expect(TokenType.SEMICOLON);
            return node;
        }

        AstNode PrintfStmt()
        {
            Expect(TokenType.KW_PRINTF);
            Expect(TokenType.LPAREN);
            Expect(TokenType.STRING_LIT);
            var node = new PrintfNode();
            while (Peek().Type == TokenType.COMMA)
            {
                Next();
                node.Expressions.Add(Expr(TokenType.COMMA, TokenType.RPAREN));
            }
            Expect(TokenType.RPAREN);
            Expect(TokenType.SEMICOLON);
            return node;
        }

        AstNode RetStmt()
        {
            Expect(TokenType.KW_RETURN);
            var e = Expr(TokenType.SEMICOLON);
            Expect(TokenType.SEMICOLON);
            var node = new PrintfNode();
            node.Expressions.Add(e);
            return node;
        }

        // ============================================================
        // Условие: <expr> [<cmpop> <expr>]
        // ============================================================
        AstNode Cond()
        {
            var left = Expr(TokenType.RPAREN,
                TokenType.OP_EQ, TokenType.OP_NEQ,
                TokenType.OP_LT, TokenType.OP_GT,
                TokenType.OP_LE, TokenType.OP_GE);

            var t = Peek();
            string? op = t.Type switch
            {
                TokenType.OP_EQ  => "==", TokenType.OP_NEQ => "!=",
                TokenType.OP_LT  => "<",  TokenType.OP_GT  => ">",
                TokenType.OP_LE  => "<=", TokenType.OP_GE  => ">=",
                _ => null
            };
            if (op == null) return left;
            Next();
            var right = Expr(TokenType.RPAREN);
            return new BinaryOpNode { Op = op, Left = left, Right = right };
        }

        // ============================================================
        // МЕТОД РАСШИРЕННОГО ПРЕДШЕСТВОВАНИЯ
        //
        // Разбирает арифметическое выражение до одного из стоп-токенов.
        //
        // Стек хранит:
        //   - терминалы (Token)
        //   - нетерминалы (AstNode) — результаты свёрток
        //   - маркер дна ($)
        //
        // На каждой итерации:
        //   1. Найти верхний терминал стека (topTerm).
        //   2. Получить отношение pred(topTerm, curInput).
        //   3. LT(<) или EQ(=) → СДВИГ.
        //   4. GT(>) → СВЁРТКА.
        //   5. оба END → конец разбора.
        //
        // Свёртка (алгоритм Грора для основы):
        //   Найти k — правый конец основы (вершина стека).
        //   Найти j — левый конец: первый символ, для которого
        //   pred(prevTerminal(j-1), symbol(j)) = LT.
        //   Свернуть stk[j..k] по правилу грамматики.
        // ============================================================
        public AstNode Expr(params TokenType[] stopAt)
        {
            // Стековые элементы
            var stk = new List<(bool isNode, TokenType tt, string val, AstNode? nd)>();
            stk.Add((false, TokenType.MARKER, "$", null));   // маркер дна

            // Счётчик открытых скобок (для отличия ) внутри от ) снаружи)
            int depth = 0;

            while (true)
            {
                // --- Найти верхний терминал стека ---
                TokenType topTT = TopTerminal(stk);

                // --- Текущий входной токен ---
                Token cur = Peek();
                bool isEnd = IsStopToken(cur.Type, depth, stopAt);
                TokenType curTT = isEnd ? TokenType.MARKER : cur.Type;

                int row = TI(topTT), col = TI(curTT);
                int rel = PT[row, col];

                // --- Оба маркера → конец разбора ---
                if (topTT == TokenType.MARKER && isEnd)
                {
                    // Стек должен содержать ровно [маркер, нетерминал]
                    if (stk.Count == 2 && stk[1].isNode)
                        return stk[1].nd!;
                    if (stk.Count == 1)
                        throw new SyntaxException(
                            $"Ожидалось выражение у '{cur.Value}' (строка {cur.Line})");
                    // Принудительная финальная свёртка
                    ReduceStack(stk);
                    if (stk.Count == 2 && stk[1].isNode) return stk[1].nd!;
                    throw new SyntaxException(
                        $"Неполное выражение у '{cur.Value}' (строка {cur.Line})");
                }

                if (rel == 1 || rel == 2)
                {
                    // СДВИГ: перенести текущий токен в стек
                    if (cur.Type == TokenType.LPAREN) depth++;
                    if (cur.Type == TokenType.RPAREN) depth--;
                    stk.Add((false, cur.Type, cur.Value, null));
                    Next();
                }
                else if (rel == 3)
                {
                    // СВЁРТКА: найти основу и применить правило
                    ReduceStack(stk);
                }
                else
                {
                    throw new SyntaxException(
                        $"Синтаксическая ошибка в выражении: неожиданный '{cur.Value}' " +
                        $"(строка {cur.Line}, поз. {cur.Column})");
                }
            }
        }

        void ReduceStack(List<(bool isNode, TokenType tt, string val, AstNode? nd)> stk)
        {
            // В операторном предшествовании основа всегда содержит хотя бы один терминал.
            //
            // Алгоритм:
            //   1. Найти крайний правый терминал (topTermIdx).
            //   2. Определить левую границу основы с учётом нетерминалов
            //      слева и справа от topTermIdx.
            //   3. Если topTerminal == ) → включить соответствующую ( в основу.

            // Шаг 1: крайний правый терминал
            int topTermIdx = stk.Count - 1;
            while (topTermIdx >= 0 && stk[topTermIdx].isNode) topTermIdx--;

            if (topTermIdx <= 0)
                throw new SyntaxException("Ошибка свёртки: основа без терминала");

            // Шаг 2: определить basisStart
            int basisStart = topTermIdx;

            // Включить нетерминал справа от topTermIdx (если есть)
            // → он уже в stk[topTermIdx+1..Count-1], мы берём до Count-1.

            // Включить нетерминал слева от topTermIdx (если есть)
            if (topTermIdx > 0 && stk[topTermIdx - 1].isNode)
                basisStart = topTermIdx - 1;

            // Если topTerminal == ')' → ищем соответствующую '(' и включаем её.
            // Это нужно для правила F → ( E ).
            if (stk[topTermIdx].tt == TokenType.RPAREN)
            {
                // Найти парную '(': двигаемся влево, считаем скобки
                int depth = 1;
                int k = topTermIdx - 1;
                while (k >= 0 && depth > 0)
                {
                    if (!stk[k].isNode)
                    {
                        if (stk[k].tt == TokenType.RPAREN) depth++;
                        if (stk[k].tt == TokenType.LPAREN) depth--;
                    }
                    k--;
                }
                basisStart = k + 1; // позиция '('
            }

            // Шаг 3: извлечь основу
            int len = stk.Count - basisStart;
            var basis = new List<(bool, TokenType, string, AstNode?)>(
                stk.GetRange(basisStart, len));
            stk.RemoveRange(basisStart, len);

            AstNode? nd = ApplyRule(basis);
            if (nd == null)
            {
                stk.AddRange(basis);
                throw new SyntaxException("Не удалось свернуть основу (нет подходящего правила)");
            }
            stk.Add((true, TokenType.EOF, "", nd));
        }

        // ============================================================
        // Применение правила грамматики к основе
        // ============================================================
        AstNode? ApplyRule(List<(bool isNode, TokenType tt, string val, AstNode? nd)> b)
        {
            int n = b.Count;
            if (n == 0) return null;

            // F → id
            if (n == 1 && !b[0].isNode && b[0].tt == TokenType.IDENTIFIER)
                return new IdentifierNode { Name = b[0].val };

            // F → num
            if (n == 1 && !b[0].isNode && b[0].tt == TokenType.NUMBER)
                return new NumberNode { Value = int.Parse(b[0].val) };

            // Нетерминал → нетерминал (цепочечное правило: T→F, E→T)
            if (n == 1 && b[0].isNode)
                return b[0].nd;

            // F → ( E )
            if (n == 3
                && !b[0].isNode && b[0].tt == TokenType.LPAREN
                && b[1].isNode
                && !b[2].isNode && b[2].tt == TokenType.RPAREN)
                return b[1].nd;

            // F → - F  (унарный минус)
            if (n == 2 && !b[0].isNode && b[0].tt == TokenType.OP_MINUS && b[1].isNode)
                return new UnaryOpNode { Op = "-", Operand = b[1].nd! };

            // F → + F  (унарный плюс — нет узла)
            if (n == 2 && !b[0].isNode && b[0].tt == TokenType.OP_PLUS && b[1].isNode)
                return b[1].nd;

            // E → E op T  (бинарная операция)
            if (n == 3 && b[0].isNode && !b[1].isNode && b[2].isNode)
            {
                string? op = b[1].tt switch
                {
                    TokenType.OP_PLUS  => "+",
                    TokenType.OP_MINUS => "-",
                    TokenType.OP_STAR  => "*",
                    TokenType.OP_SLASH => "/",
                    _ => null
                };
                if (op != null)
                    return new BinaryOpNode { Op = op, Left = b[0].nd!, Right = b[2].nd! };
            }

            return null;
        }

        // ============================================================
        // Вспомогательные методы
        // ============================================================

        /// <summary>Найти верхний терминал в стеке (пропустить нетерминалы).</summary>
        TokenType TopTerminal(List<(bool isNode, TokenType tt, string val, AstNode? nd)> stk)
        {
            for (int i = stk.Count - 1; i >= 0; i--)
                if (!stk[i].isNode) return stk[i].tt;
            return TokenType.MARKER;
        }

        /// <summary>Является ли токен стоп-символом (концом выражения)?</summary>
        bool IsStopToken(TokenType tt, int depth, TokenType[] stopAt)
        {
            if (tt == TokenType.EOF) return true;
            // Явно указанные стоп-символы (кроме ) при depth>0)
            foreach (var s in stopAt)
            {
                if (tt == s)
                {
                    // ) может быть стоп-токеном только при depth==0
                    if (tt == TokenType.RPAREN && depth > 0) continue;
                    return true;
                }
            }
            // Символы, которые никогда не входят в арифм. выражение
            if (!IsArithToken(tt)) return true;
            return false;
        }

        bool IsArithToken(TokenType tt) =>
            tt == TokenType.IDENTIFIER || tt == TokenType.NUMBER ||
            tt == TokenType.OP_PLUS    || tt == TokenType.OP_MINUS ||
            tt == TokenType.OP_STAR    || tt == TokenType.OP_SLASH ||
            tt == TokenType.LPAREN     || tt == TokenType.RPAREN;

        int TI(TokenType tt) => tt switch
        {
            TokenType.IDENTIFIER => I_ID,
            TokenType.NUMBER     => I_ID,
            TokenType.OP_PLUS    => I_ADD,
            TokenType.OP_MINUS   => I_SUB,
            TokenType.OP_STAR    => I_MUL,
            TokenType.OP_SLASH   => I_DIV,
            TokenType.LPAREN     => I_LP,
            TokenType.RPAREN     => I_RP,
            _                    => I_END,
        };
    }
}
