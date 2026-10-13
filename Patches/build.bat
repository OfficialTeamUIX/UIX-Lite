@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build bin\uixlite.bin and bin\symbols.txt.
rem Requires clang, ld.lld, llvm-objcopy, and llvm-nm.

cd /d "%~dp0"

if not defined CC set "CC=clang"
if not defined LD set "LD=ld.lld"
if not defined OBJCOPY set "OBJCOPY=llvm-objcopy"
if not defined NM set "NM=llvm-nm"

if not defined OPT set "OPT=-O2"
set "CFLAGS=--target=i386-unknown-none-elf -ffreestanding -fno-pic -fno-pie %OPT% -fno-builtin -fno-stack-protector -mno-sse -mno-mmx -fno-zero-initialized-in-bss -fno-asynchronous-unwind-tables -fno-exceptions -Wall"

if not exist "build" mkdir "build"
if errorlevel 1 goto :error

if not exist "bin" mkdir "bin"
if errorlevel 1 goto :error

del /q "build\*.o" 2>nul

set "OBJECTS="

for %%F in ("src\*.c") do (
    echo Compiling %%~nxF...

    "%CC%" %CFLAGS% -c "%%~fF" -o "build\%%~nF.o"
    if errorlevel 1 goto :error

    set OBJECTS=!OBJECTS! "build\%%~nF.o"
)

echo Linking uixlite.elf...

"%LD%" -m elf_i386 -T "link.ld" -o "build\uixlite.elf" !OBJECTS!
if errorlevel 1 goto :error

echo Creating uixlite.bin...

"%OBJCOPY%" -O binary "build\uixlite.elf" "bin\uixlite.bin"
if errorlevel 1 goto :error

echo Generating symbols.txt...

"%NM%" "build\uixlite.elf" > "build\symbols-all.txt"
if errorlevel 1 goto :error

findstr /R /C:" [TtDd] patch_" "build\symbols-all.txt" > "bin\symbols.txt"
if errorlevel 1 (
    echo No patch symbols were found.
    goto :error
)

type "bin\symbols.txt"

for %%F in ("bin\uixlite.bin") do (
    echo %%~fF: %%~zF bytes
)

echo Build completed successfully.
exit /b 0

:error
echo Build failed.
exit /b 1
