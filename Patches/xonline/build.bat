@echo off
setlocal EnableExtensions
cd /d "%~dp0"
if not defined CC set "CC=clang"
if not defined LD set "LD=ld.lld"
if not defined OBJCOPY set "OBJCOPY=llvm-objcopy"
if not defined NM set "NM=llvm-nm"
if not exist build mkdir build
if not exist bin mkdir bin
%CC% --target=i386-unknown-none-elf -ffreestanding -fno-pic -fno-pie -O2 -fno-builtin -fno-stack-protector -mno-sse -mno-mmx -fno-zero-initialized-in-bss -fno-asynchronous-unwind-tables -fno-exceptions -Wall -c src\xonline_skin.c -o build\xonline_skin.o || exit /b 1
%LD% -m elf_i386 -T link.ld -o build\xonline_skin.elf build\xonline_skin.o || exit /b 1
%OBJCOPY% -O binary build\xonline_skin.elf bin\xonline_skin.bin || exit /b 1
%NM% build\xonline_skin.elf > build\symbols-all.txt || exit /b 1
findstr /R /C:" [TtDd] patch_" build\symbols-all.txt > bin\symbols.txt || exit /b 1
type bin\symbols.txt
exit /b 0
