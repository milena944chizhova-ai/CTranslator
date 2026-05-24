using System.Collections.Generic;
using System.IO;

namespace CTranslator
{
    // -------------------------------------------------------
    // Базовый класс всех узлов синтаксического дерева (AST)
    // -------------------------------------------------------
    public abstract class AstNode
    {
        /// <summary>Печатает поддерево в указанный поток с отступом.</summary>
        public abstract void Print(TextWriter w, string prefix = "", bool last = true);

        protected static string Connector(bool last) => last ? "└── " : "├── ";
        protected static string ChildPrefix(string prefix, bool last)
            => prefix + (last ? "    " : "│   ");
    }

    // -------------------------------------------------------
    // Составной оператор / тело программы
    // -------------------------------------------------------
    public class BlockNode : AstNode
    {
        public List<AstNode> Statements { get; } = new List<AstNode>();

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + "Block");
            string childPfx = ChildPrefix(prefix, last);
            for (int i = 0; i < Statements.Count; i++)
                Statements[i].Print(w, childPfx, i == Statements.Count - 1);
        }
    }

    // -------------------------------------------------------
    // Объявление переменной: int <name>;
    // -------------------------------------------------------
    public class DeclarationNode : AstNode
    {
        public string Name { get; set; } = "";

        public override void Print(TextWriter w, string prefix = "", bool last = true)
            => w.WriteLine(prefix + Connector(last) + $"Declaration(int {Name})");
    }

    // -------------------------------------------------------
    // Оператор присваивания: <target> = <value>;
    // -------------------------------------------------------
    public class AssignNode : AstNode
    {
        public string  Target { get; set; } = "";
        public AstNode Value  { get; set; } = null!;

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + $"Assign({Target})");
            string childPfx = ChildPrefix(prefix, last);
            Value.Print(w, childPfx, true);
        }
    }

    // -------------------------------------------------------
    // Условный оператор: if (<cond>) <then> [else <else>]
    // -------------------------------------------------------
    public class IfNode : AstNode
    {
        public AstNode  Condition  { get; set; } = null!;
        public AstNode  ThenBranch { get; set; } = null!;
        public AstNode? ElseBranch { get; set; }

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + "If");
            string childPfx = ChildPrefix(prefix, last);
            bool hasElse = ElseBranch != null;

            w.WriteLine(childPfx + "├── Condition:");
            Condition.Print(w, childPfx + "│   ", true);

            w.WriteLine(childPfx + (hasElse ? "├── " : "└── ") + "Then:");
            ThenBranch.Print(w, childPfx + (hasElse ? "│   " : "    "), true);

            if (hasElse)
            {
                w.WriteLine(childPfx + "└── Else:");
                ElseBranch!.Print(w, childPfx + "    ", true);
            }
        }
    }

    // -------------------------------------------------------
    // Оператор ввода: scanf("%d", &<var>, ...)
    // -------------------------------------------------------
    public class ScanfNode : AstNode
    {
        public List<string> Variables { get; } = new List<string>();

        public override void Print(TextWriter w, string prefix = "", bool last = true)
            => w.WriteLine(prefix + Connector(last) +
                $"Scanf([{string.Join(", ", Variables)}])");
    }

    // -------------------------------------------------------
    // Оператор вывода: printf("%d\n", <expr>, ...)
    // -------------------------------------------------------
    public class PrintfNode : AstNode
    {
        public List<AstNode> Expressions { get; } = new List<AstNode>();

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + "Printf");
            string childPfx = ChildPrefix(prefix, last);
            for (int i = 0; i < Expressions.Count; i++)
                Expressions[i].Print(w, childPfx, i == Expressions.Count - 1);
        }
    }

    // -------------------------------------------------------
    // Бинарная операция: <left> <op> <right>
    // -------------------------------------------------------
    public class BinaryOpNode : AstNode
    {
        public string  Op    { get; set; } = "";
        public AstNode Left  { get; set; } = null!;
        public AstNode Right { get; set; } = null!;

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + $"BinaryOp({Op})");
            string childPfx = ChildPrefix(prefix, last);
            Left.Print(w,  childPfx, false);
            Right.Print(w, childPfx, true);
        }
    }

    // -------------------------------------------------------
    // Унарная операция: <op><operand>
    // -------------------------------------------------------
    public class UnaryOpNode : AstNode
    {
        public string  Op      { get; set; } = "";
        public AstNode Operand { get; set; } = null!;

        public override void Print(TextWriter w, string prefix = "", bool last = true)
        {
            w.WriteLine(prefix + Connector(last) + $"UnaryOp({Op})");
            Operand.Print(w, ChildPrefix(prefix, last), true);
        }
    }

    // -------------------------------------------------------
    // Целочисленная константа
    // -------------------------------------------------------
    public class NumberNode : AstNode
    {
        public int Value { get; set; }

        public override void Print(TextWriter w, string prefix = "", bool last = true)
            => w.WriteLine(prefix + Connector(last) + $"Number({Value})");
    }

    // -------------------------------------------------------
    // Идентификатор (имя переменной)
    // -------------------------------------------------------
    public class IdentifierNode : AstNode
    {
        public string Name { get; set; } = "";

        public override void Print(TextWriter w, string prefix = "", bool last = true)
            => w.WriteLine(prefix + Connector(last) + $"Identifier({Name})");
    }
}
