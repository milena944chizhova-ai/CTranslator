using System;
using System.Collections.Generic;

namespace CTranslator
{
    // -------------------------------------------------------
    // Исключение времени выполнения
    // -------------------------------------------------------
    public class RuntimeException : Exception
    {
        public RuntimeException(string msg) : base(msg) { }
    }

    // -------------------------------------------------------
    // Интерпретатор
    //
    // Выполняет программу по синтаксическому дереву AST.
    // Все переменные хранятся в словаре _mem (имя → значение?).
    // Null означает «не инициализировано».
    // -------------------------------------------------------
    public class Interpreter
    {
        private readonly Dictionary<string, int?> _mem =
            new Dictionary<string, int?>();

        /// <summary>Запустить выполнение программы от корневого узла.</summary>
        public bool Execute(AstNode root)
        {
            try
            {
                Exec(root);
                return true;
            }
            catch (RuntimeException ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"\nОшибка времени выполнения: {ex.Message}");
                Console.ResetColor();
                return false;
            }
        }

        // -------------------------------------------------------
        // Выполнение оператора
        // -------------------------------------------------------
        private void Exec(AstNode node)
        {
            switch (node)
            {
                case BlockNode blk:
                    foreach (var s in blk.Statements) Exec(s);
                    break;

                case DeclarationNode d:
                    _mem[d.Name] = null; // объявлена, но не инициализирована
                    break;

                case AssignNode a:
                    _mem[a.Target] = Eval(a.Value);
                    break;

                case IfNode ifn:
                    if (Eval(ifn.Condition) != 0)
                        Exec(ifn.ThenBranch);
                    else if (ifn.ElseBranch != null)
                        Exec(ifn.ElseBranch);
                    break;

                case ScanfNode sc:
                    foreach (var v in sc.Variables)
                    {
                        Console.Write($"Введите значение для {v}: ");
                        string? line = Console.ReadLine();
                        if (!int.TryParse(line?.Trim(), out int val))
                            throw new RuntimeException(
                                $"Некорректный ввод для переменной '{v}'");
                        _mem[v] = val;
                    }
                    break;

                case PrintfNode pr:
                    foreach (var e in pr.Expressions)
                        Console.WriteLine(Eval(e));
                    break;

                default:
                    throw new RuntimeException(
                        $"Неизвестный тип узла '{node.GetType().Name}'");
            }
        }

        // -------------------------------------------------------
        // Вычисление выражения → int
        // -------------------------------------------------------
        private int Eval(AstNode node)
        {
            return node switch
            {
                NumberNode n =>
                    n.Value,

                IdentifierNode id =>
                    _mem.TryGetValue(id.Name, out var v) && v.HasValue
                        ? v.Value
                        : throw new RuntimeException(
                              $"Переменная '{id.Name}' не инициализирована"),

                BinaryOpNode b =>
                    b.Op switch
                    {
                        "+"  => Eval(b.Left) + Eval(b.Right),
                        "-"  => Eval(b.Left) - Eval(b.Right),
                        "*"  => Eval(b.Left) * Eval(b.Right),
                        "/"  => Eval(b.Right) == 0
                                  ? throw new RuntimeException("Деление на ноль")
                                  : Eval(b.Left) / Eval(b.Right),
                        "==" => Eval(b.Left) == Eval(b.Right) ? 1 : 0,
                        "!=" => Eval(b.Left) != Eval(b.Right) ? 1 : 0,
                        "<"  => Eval(b.Left) <  Eval(b.Right) ? 1 : 0,
                        ">"  => Eval(b.Left) >  Eval(b.Right) ? 1 : 0,
                        "<=" => Eval(b.Left) <= Eval(b.Right) ? 1 : 0,
                        ">=" => Eval(b.Left) >= Eval(b.Right) ? 1 : 0,
                        _    => throw new RuntimeException(
                                    $"Неизвестный оператор '{b.Op}'")
                    },

                UnaryOpNode u =>
                    u.Op == "-" ? -Eval(u.Operand) : Eval(u.Operand),

                _ => throw new RuntimeException(
                         $"Неизвестный тип узла '{node.GetType().Name}'")
            };
        }

        // -------------------------------------------------------
        // Вывод текущего состояния памяти
        // -------------------------------------------------------
        public void PrintMemory()
        {
            Console.WriteLine("\n=== Состояние памяти ===");
            foreach (var (k, v) in _mem)
                Console.WriteLine(
                    $"  {k,-10} = " +
                    $"{(v.HasValue ? v.Value.ToString() : "<не инициализировано>")}");
        }
    }
}
