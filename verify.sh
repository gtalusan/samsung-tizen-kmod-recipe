#!/bin/bash
# Build-artifact verification: every wall the TV loader enforces, checked
# locally before the module ever reaches the TV.
set -euo pipefail
cd "$(dirname "$0")"
KO=${1:-out/research_kmem.ko}
CROSS=${CROSS_COMPILE:-arm-none-eabi-}
NM=${NM:-${CROSS}nm}
READELF=${READELF:-${CROSS}readelf}

[ -f "$KO" ] || { echo "error: $KO missing — run ./build.sh first" >&2; exit 1; }

# 1. vermagic: the TV's exact string (DTV_KERNEL_VERSION form, trailing space)
python3 - "$KO" <<'EOF' || { echo "FAIL: vermagic" >&2; exit 1; }
import sys
d = open(sys.argv[1], "rb").read()
assert b"vermagic=0000000, release SMP preempt mod_unload ARMv7 p2v8 " in d
EOF

# 2. required sections: the TV loader demands the module-PLT sections
"$READELF" -S "$KO" | grep -q "\.core\.plt" || { echo "FAIL: .core.plt missing" >&2; exit 1; }
"$READELF" -S "$KO" | grep -q "\.init\.plt" || { echo "FAIL: .init.plt missing" >&2; exit 1; }

# 3. no libgcc unwinder references (else "Unknown symbol in module")
if "$NM" -u "$KO" | grep -qE "__aeabi_unwind_cpp_pr[01]"; then
	echo "FAIL: __aeabi_unwind_cpp_pr0/pr1 undefined (need -fno-unwind-tables)" >&2
	exit 1
fi

# 4. reloc types: 47/48 (Thumb MOVW/MOVT) are refused by the TV loader
python3 - "$KO" <<'EOF' || { echo "FAIL: unsupported reloc type" >&2; exit 1; }
import struct, sys
d = open(sys.argv[1], "rb").read()
shoff = struct.unpack_from("<I", d, 32)[0]
shentsize = struct.unpack_from("<H", d, 46)[0]
shnum = struct.unpack_from("<H", d, 48)[0]
shstrndx = struct.unpack_from("<H", d, 50)[0]
def sh(i):
	o = shoff + i * shentsize
	return struct.unpack_from("<IIIIIIIIII", d, o)
no, ns = sh(shstrndx)[4], sh(shstrndx)[5]
names = d[no:no+ns]
def nm(x):
	e = names.find(b"\x00", x)
	return names[x:e].decode()
types = {}
for i in range(shnum):
	h = sh(i)
	if nm(h[0]).startswith(".rel"):
		for j in range(0, h[5], 8):
			t = struct.unpack_from("<I", d, h[4]+j+4)[0] & 0xff
			types[t] = types.get(t, 0) + 1
bad = [t for t in types if t in (47, 48)]
assert not bad, f"refused reloc types {bad}"
print("reloc types:", types)
EOF

echo "verify OK: vermagic, .core.plt/.init.plt, no unwind_cpp, no 47/48 relocs"
