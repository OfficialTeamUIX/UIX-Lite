#!/bin/sh
# Build the patch code: bin/uixlite.bin (loaded at 0x001fe000) and
# bin/symbols.txt, which patch.py reads. Needs clang and ld.lld (LLVM).
#
#   ./build.sh
set -e
cd "$(dirname "$0")"
CC=${CC:-clang}
LD=${LD:-ld.lld}
OBJCOPY=${OBJCOPY:-llvm-objcopy}
NM=${NM:-llvm-nm}
CFLAGS="--target=i386-unknown-none-elf -ffreestanding -fno-pic -fno-pie -O2 -fno-builtin \
 -fno-stack-protector -mno-sse -mno-mmx -fno-zero-initialized-in-bss \
 -fno-asynchronous-unwind-tables -fno-exceptions -Wall"
mkdir -p build bin
rm -f build/*.o
for f in src/*.c; do
	$CC $CFLAGS -c "$f" -o "build/$(basename "$f" .c).o"
done
$LD -m elf_i386 -T link.ld -o build/uixlite.elf build/*.o
$OBJCOPY -O binary build/uixlite.elf bin/uixlite.bin
$NM build/uixlite.elf | grep -E " [TtDd] patch_" > bin/symbols.txt
cat bin/symbols.txt
ls -l bin/uixlite.bin
