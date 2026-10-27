#!/bin/sh
# verify-uep-gates.sh — run ON the TV (or via ssh) to confirm the UEP gate
# patch is live in kernel memory, using the recipe module's init-time dump
# path (probe_kernel_read + printk).
#
# The TV cannot unload modules (rmmod is EBUSY on every module), so this
# verifies all four addresses from a SINGLE insmod via the dump_vas list.
# The module must not already be loaded — a fresh boot, or nothing loaded.
#
# IMPORTANT: do NOT read /dev/kmem with dd — the file-read fop crashes the
# TV kernel (copy_to_user under Tizen's SMACK uaccess). dump_vas is the
# reliable read path.
#
# Usage (on the TV, UEP down, module uploaded, run by content):
#   sh -c "$(cat verify-uep-gates.sh)" /opt/usr/share/tvroot/research_kmem.ko
#   (script-FILE loading is UEP-refused on this TV — always content-exec)
#
# Expects: the three gate sites read `00 30 a0 e3` (patched `mov r3, #0`)
# and s_uepStatus reads `01 00 00 00` (the RAM-only patch never writes it).
MOD=${1:-/opt/usr/share/tvroot/research_kmem.ko}
export PATH=/bin:/sbin:/usr/bin:/usr/sbin

VAS="0xc01de10c:0xc01de23c:0xc01de524:0xc069342c"   # 3 gates + s_uepStatus (colons: param parsing splits on commas)
STATUS="c069342c"
ok=1

if lsmod | grep -q research_kmem; then
	echo "FAIL: research_kmem already loaded — the TV cannot unload modules; reboot the TV first"
	exit 1
fi

insmod "$MOD" enable=1 "dump_vas=$VAS" 2>/dev/null || {
	echo "FAIL: insmod failed (UEP down?)"
	exit 1
}

for a in c01de10c c01de23c c01de524 $STATUS; do
	line=$(dmesg | grep "dump@0x$a " | tail -1)
	bytes=$(echo "$line" | sed 's/.*rc=[0-9-]*: //')
	if [ "$a" = "$STATUS" ]; then
		expect="01 00 00 00"
	else
		expect="00 30 a0 e3"
	fi
	if echo "$bytes" | grep -q "^$expect"; then
		echo "PASS $a: $bytes"
	else
		echo "FAIL $a: $bytes (expected $expect)"
		ok=0
	fi
done

if [ "$ok" -eq 1 ]; then
	echo "UEP GATES: all three patched, s_uepStatus untouched"
else
	echo "UEP GATES: FAILURES — see above"
	exit 1
fi
