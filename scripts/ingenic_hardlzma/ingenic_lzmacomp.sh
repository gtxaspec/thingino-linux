#!/bin/bash
# Without this the final sync masks a failing lzma and the caller's
# "|| (rm -f $@; false)" guard never fires, so the build dies later in mkimage
# with a misleading "Can't open ... .jzlzma".
set -e
cp $1 arch/mips/boot/vmlinux.bin.bk
./scripts/ingenic_hardlzma/lzma -z -k -f -9 arch/mips/boot/vmlinux.bin.bk
cp jz_lzma_out.bin $2
rm jz_lzma_out.bin arch/mips/boot/vmlinux.bin.bk
sync

