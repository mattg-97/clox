@echo off
setlocal enabledelayedexpansion

set CC=clang
set CFLAGS=-g 
set SRCDIR=src
set OBJDIR=obj
set BINDIR=bin
set TARGET=%BINDIR%\clox.exe

:: Find all .c files recursively
for /r %SRCDIR% %%F in (*.c) do (
    set "SRCS=!SRCS! %%F"
)

:: Generate corresponding .o file names
set "OBJS="
for %%S in (%SRCS%) do (
    set "OBJ=%%S"
    set "OBJ=!OBJ:%SRCDIR%=%OBJDIR%!"
    set "OBJ=!OBJ:.c=.o!"
    set "OBJS=!OBJS! !OBJ!"
)

:: Generate include directories
set "INCLUDES=-I%SRCDIR%"
for /d /r %SRCDIR%\lib %%D in (*) do (
    set "INCLUDES=!INCLUDES! -I%%D"
)

if "%1"=="" goto all
if "%1"=="all" goto all
if "%1"=="clean" goto clean
if "%1"=="run" goto run
if "%1"=="debug" goto debug

:all
call :compile
goto :eof

:compile
if not exist %BINDIR% mkdir %BINDIR%
for %%S in (%SRCS%) do (
    set "OBJ=%%S"
    set "OBJ=!OBJ:%SRCDIR%=%OBJDIR%!"
    set "OBJ=!OBJ:.c=.o!"
    if not exist !OBJ:\=^/! (
        if not exist "!OBJ:~0,-2!" mkdir "!OBJ:~0,-2!"
        %CC% %CFLAGS% %INCLUDES% -c %%S -o !OBJ!
    )
)
%CC% %CFLAGS% %OBJS% -o %TARGET%
goto :eof

:run
call :compile
%TARGET%
goto :eof

:clean
if exist %OBJDIR% rmdir /s /q %OBJDIR%
if exist %BINDIR% rmdir /s /q %BINDIR%
goto :eof

:debug
call :compile
lldb %TARGET%
goto :eof

:eof