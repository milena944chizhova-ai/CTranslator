@echo off
REM build.bat — сборка транслятора под Windows (GCC / MinGW)
REM Запуск: build.bat  [после чего: translator.exe tests\input.txt]

set CC=gcc
set CFLAGS=-std=c99 -Wall -Wextra -pedantic -Isrc

%CC% %CFLAGS% ^
  src\main.c ^
  src\token.c ^
  src\lexer.c ^
  src\ast.c ^
  src\parser.c ^
  src\semantic.c ^
  src\interpreter.c ^
  -o translator.exe

if %ERRORLEVEL% EQU 0 (
    echo Сборка успешна: translator.exe
    echo Запуск: translator.exe tests\input.txt
) else (
    echo Ошибка сборки!
)
