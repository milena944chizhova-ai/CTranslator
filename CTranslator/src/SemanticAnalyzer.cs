using System;
using System.Collections.Generic;
using System.IO;

namespace CTranslator
{
    // -------------------------------------------------------
    // Запись таблицы символов
    // -------------------------------------------------------
    public class SymbolEntry
    {
        public string Name          { get; }
        public string Type          { get; }
        public bool   IsInitialized { get; set; }

        public SymbolEntry(string name, string type)
        {
            Name = name; Type = type; IsInitialized = false;
        }
    }

    // -------------------------------------------------------
    // Семантический анализатор
    //
    // Обходит AST в глубину и выполняет проверки:
    //   - объявление до использования;
    //   - повторное объявление;
    //   - инициализация перед использованием.
    // Ведёт таблицу символов (словарь SymbolEntry).
    // -------------------------------------------------------
    public class SemanticAnalyzer
    {
        public Dictionary<string, SymbolEntry> Symbols { get; } =
            new Dictionary<string, SymbolEntry>();

        public List<string> Errors { get; } = new List<string>();

        public void Analyze(AstNode root) => Visit(root);

        private void Visit(AstNode node)
        {
            switch (node)
            {
                case BlockNode blk:
                    foreach (var s in blk.Statements) Visit(s);
                    break;

                case DeclarationNode d:
                    if (Symbols.ContainsKey(d.Name))
                        Errors.Add($"Семантическая ошибка: повторное объявление " +
                                   $"переменной '{d.Name}'");
                    else
                        Symbols[d.Name] = new SymbolEntry(d.Name, "int");
                    break;

                case AssignNode a:
                    if (!Symbols.ContainsKey(a.Target))
                        Errors.Add($"Семантическая ошибка: переменная '{a.Target}' " +
                                   $"не объявлена");
                    Visit(a.Value);
                    if (Symbols.ContainsKey(a.Target))
                        Symbols[a.Target].IsInitialized = true;
                    break;

                case IdentifierNode id:
                    if (!Symbols.ContainsKey(id.Name))
                        Errors.Add($"Семантическая ошибка: переменная '{id.Name}' " +
                                   $"не объявлена");
                    else if (!Symbols[id.Name].IsInitialized)
                        Errors.Add($"Семантическая ошибка: переменная '{id.Name}' " +
                                   $"использована до инициализации");
                    break;

                case IfNode ifn:
                    Visit(ifn.Condition);
                    Visit(ifn.ThenBranch);
                    if (ifn.ElseBranch != null) Visit(ifn.ElseBranch);
                    break;

                case ScanfNode sc:
                    foreach (var v in sc.Variables)
                    {
                        if (!Symbols.ContainsKey(v))
                            Errors.Add($"Семантическая ошибка: переменная '{v}' " +
                                       $"не объявлена");
                        else
                            Symbols[v].IsInitialized = true;
                    }
                    break;

                case PrintfNode pr:
                    foreach (var e in pr.Expressions) Visit(e);
                    break;

                case BinaryOpNode b:
                    Visit(b.Left); Visit(b.Right);
                    break;

                case UnaryOpNode u:
                    Visit(u.Operand);
                    break;

                case NumberNode:
                    break; // константы не проверяются

                default:
                    break;
            }
        }

        // -------------------------------------------------------
        // Вывод таблицы символов
        // -------------------------------------------------------
        public void PrintTable(TextWriter w)
        {
            w.WriteLine("=== ТАБЛИЦА СИМВОЛОВ ===");
            w.WriteLine($"{"Имя",-12}{"Тип",-8}{"Объявлена",-14}{"Инициализирована",-20}");
            w.WriteLine(new string('-', 54));
            foreach (var s in Symbols.Values)
                w.WriteLine($"{s.Name,-12}{s.Type,-8}{"да",-14}" +
                    $"{(s.IsInitialized ? "да" : "нет"),-20}");
            w.WriteLine();
            if (Errors.Count == 0)
                w.WriteLine("=== Семантических ошибок не обнаружено ===");
            else
            {
                w.WriteLine($"=== Обнаружено семантических ошибок: {Errors.Count} ===");
                foreach (var e in Errors) w.WriteLine("  " + e);
            }
        }
    }
}
