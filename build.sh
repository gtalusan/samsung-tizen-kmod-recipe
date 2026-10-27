#!/bin/bash
# Build research_kmem.ko for Samsung Tizen 5.5 TVs (Linux 4.1.10, ARMv7).
# One shot: ./build.sh   (or: docker build --output out .)
#
# The flags below are the field-verified set (captured from the kernel
# build's .research_kmem.o.cmd on 2026-10-26), trimmed to what matters:
#   - ARM mode (-marm), NOT Thumb: the TV loader refuses Thumb MOVW/MOVT
#     relocs (47/48) with "unknown relocation: 47"
#   - -fno-unwind-tables: otherwise libgcc's __aeabi_unwind_cpp_pr0/pr1
#     become undefined symbols ("Unknown symbol in module")
#   - -DCONFIG_MODULES -DCONFIG_MODULE_UNLOAD: module.h paths + rmmod support
set -euo pipefail
cd "$(dirname "$0")"

CROSS=${CROSS_COMPILE:-arm-none-eabi-}
CC=${CC:-${CROSS}gcc}
LD=${LD:-${CROSS}ld}

if ! command -v "$CC" >/dev/null 2>&1; then
	echo "error: $CC not found (set CROSS_COMPILE or CC)" >&2
	exit 1
fi

GCCINC=$("$CC" -print-file-name=include)

CFLAGS=(
	-nostdinc -isystem "$GCCINC"
	-I./arch/arm/include -Iarch/arm/include/generated/uapi -Iarch/arm/include/generated
	-Iinclude -I./arch/arm/include/uapi -Iarch/arm/include/generated/uapi
	-I./include/uapi -Iinclude/generated/uapi
	-include ./include/linux/kconfig.h
	-D__KERNEL__ -mlittle-endian -fno-strict-aliasing -fno-common
	-Werror-implicit-function-declaration -std=gnu89 -fno-dwarf2-cfi-asm
	-mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfp
	-D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm
	-fno-delete-null-pointer-checks -O2 -fno-section-anchors -fno-stack-protector
	-fomit-frame-pointer -fconserve-stack -DCC_HAVE_ASM_GOTO
	-fno-unwind-tables -fno-asynchronous-unwind-tables
	-marm -mlong-calls -DCONFIG_MODULES -DCONFIG_MODULE_UNLOAD
	-D'KBUILD_MODNAME="research_kmem"' -DMODULE
)

mkdir -p out
(
	cd kernel-headers
	"$CC" "${CFLAGS[@]}" -Wall -c ../research_kmem.c -o research_kmem.o
	"${CROSS}as" -o plt.o ../plt.S
	"$LD" -r -o ../out/research_kmem.ko research_kmem.o plt.o
)
./verify.sh
echo "built out/research_kmem.ko"
