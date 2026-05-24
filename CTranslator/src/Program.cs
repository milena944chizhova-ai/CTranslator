using System;
using System.IO;
using System.Collections.Generic;

namespace CTranslator
{
    class Program
    {
        static int Main(string[] args)
        {
            string inputFile = args.Length > 0 ? args[0] : "input.txt";

            Console.WriteLine("╔══════════════════════════════════════════════════╗");
            Console.WriteLine("║  Транслятор подмножества C  (вариант 58, код 222253) ║");
            Console.WriteLine("║  Метод разбора: расширенное предшествование      ║");
            Console.WriteLine("║  Промежуточный код: AST   Тип: интерпретатор    ║");
            Console.WriteLine("╚══════════════════════════════════════════════════╝");
            Console.WriteLine();

            if (!File.Exists(inputFile))
            {
                Console.WriteLine($"Ошибка: файл '{inputFile}' не найден.");
                Console.WriteLine("Использование: CTranslator [input.txt]");
                return 1;
            }

            string source = File.ReadAllText(inputFile);
            Console.WriteLine($"Входной файл: {inputFile}");
            Console.WriteLine();

            // ====================================================
            // 1. ЛЕКСИЧЕСКИЙ АНАЛИЗ
            // ====================================================
            Console.WriteLine("─── 1. Лексический анализ ───");
            var lexer = new Lexer(source);
            var (tokens, lexErrors) = lexer.Tokenize();

            // Запись таблицы лексем в файл
            using (var lw = new StreamWriter("lexer_output.txt", false,
                       System.Text.Encoding.UTF8))
            {
                lw.WriteLine("=== ТАБЛИЦА ЛЕКСЕМ ===");
                lw.WriteLine($"{"№",-5}{"Тип",-16}{"Значение",-16}{"Строка",-8}{"Позиция"}");
                lw.WriteLine(new string('─', 55));
                for (int i = 0; i < tokens.Count; i++)
                    lw.WriteLine(
                        $"{i + 1,-5}{tokens[i].Type,-16}{tokens[i].Value,-16}" +
                        $"{tokens[i].Line,-8}{tokens[i].Column}");
                lw.WriteLine();
                if (lexErrors.Count == 0)
                    lw.WriteLine("=== Лексических ошибок не обнаружено ===");
                else
                {
                    lw.WriteLine($"=== Лексических ошибок: {lexErrors.Count} ===");
                    foreach (var e in lexErrors) lw.WriteLine("  " + e);
                }
            }

            Console.WriteLine($"  Всего лексем: {tokens.Count}");
            if (lexErrors.Count > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"  Лексических ошибок: {lexErrors.Count}");
                foreach (var e in lexErrors) Console.WriteLine("    " + e);
                Console.ResetColor();
                Console.WriteLine("  Таблица лексем → lexer_output.txt");
                return 2;
            }
            Console.WriteLine("  Лексических ошибок не обнаружено.");
            Console.WriteLine("  Таблица лексем → lexer_output.txt");
            Console.WriteLine();

            // ====================================================
            // 2. СИНТАКСИЧЕСКИЙ АНАЛИЗ (расширенное предшествование)
            // ====================================================
            Console.WriteLine("─── 2. Синтаксический анализ (расширенное предшествование) ───");
            AstNode? ast = null;
            try
            {
                var parser = new PrecedenceParser(tokens);
                ast = parser.Parse();
            }
            catch (SyntaxException ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("  " + ex.Message);
                Console.ResetColor();
                return 3;
            }

            // Запись AST в файл
            using (var pw = new StreamWriter("ast_output.txt", false,
                       System.Text.Encoding.UTF8))
            {
                pw.WriteLine("=== СИНТАКСИЧЕСКОЕ ДЕРЕВО (AST) ===");
                pw.WriteLine();
                ast.Print(pw, "", true);
                pw.WriteLine();
                pw.WriteLine("=== Синтаксических ошибок не обнаружено ===");
            }
            Console.WriteLine("  Синтаксический анализ завершён успешно.");
            Console.WriteLine("  Синтаксическое дерево → ast_output.txt");
            Console.WriteLine();

            // ====================================================
            // 3. СЕМАНТИЧЕСКИЙ АНАЛИЗ
            // ====================================================
            Console.WriteLine("─── 3. Семантический анализ ───");
            var sem = new SemanticAnalyzer();
            sem.Analyze(ast);

            using (var sw = new StreamWriter("symbols_output.txt", false,
                       System.Text.Encoding.UTF8))
                sem.PrintTable(sw);

            if (sem.Errors.Count > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"  Семантических ошибок: {sem.Errors.Count}");
                foreach (var e in sem.Errors) Console.WriteLine("    " + e);
                Console.ResetColor();
                Console.WriteLine("  Таблица символов → symbols_output.txt");
                return 4;
            }
            Console.WriteLine($"  Переменных объявлено: {sem.Symbols.Count}");
            Console.WriteLine("  Семантических ошибок не обнаружено.");
            Console.WriteLine("  Таблица символов → symbols_output.txt");
            Console.WriteLine();

            // ====================================================
            // 4. ИНТЕРПРЕТАЦИЯ
            // ====================================================
            Console.WriteLine("─── 4. Выполнение программы ───");
            Console.WriteLine();
            var interp = new Interpreter();
            bool ok = interp.Execute(ast);
            interp.PrintMemory();
            if (!ok)
            {
                Console.WriteLine();
                Console.WriteLine("─── Выполнение завершено с ошибкой ───");
                return 5;
            }
            Console.WriteLine();
            Console.WriteLine("─── Трансляция и выполнение завершены ───");
            return 0;
        }
    }
}
