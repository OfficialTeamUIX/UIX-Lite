#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"

CC=${CC:-clang}
LD=${LD:-ld.lld}
OBJCOPY=${OBJCOPY:-llvm-objcopy}
NM=${NM:-llvm-nm}

mkdir -p build bin
"$CC" --target=i386-unknown-none-elf -ffreestanding -fno-pic -fno-pie -O2 \
  -fno-builtin -fno-stack-protector -mno-sse -mno-mmx \
  -fno-zero-initialized-in-bss -fno-asynchronous-unwind-tables \
  -fno-exceptions -Wall -c src/xonline_skin.c -o build/xonline_skin.o
"$LD" -m elf_i386 -T link.ld -o build/xonline_skin.elf build/xonline_skin.o
"$OBJCOPY" -O binary build/xonline_skin.elf bin/xonline_skin.bin
"$NM" build/xonline_skin.elf > build/symbols-all.txt
grep -E ' [TtDd] patch_' build/symbols-all.txt > bin/symbols.txt
cat bin/symbols.txt
