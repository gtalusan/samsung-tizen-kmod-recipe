# samsung-tizen-kmod-recipe

The minimal, field-verified recipe for building a loadable kernel module
for Samsung Tizen 5.5 TVs — Linux `4.1.10`, `T-NKMAKUC` family (2020 QLED,
NikeM/NikeL), ARMv7. Verified 2026-10-26 on a QN55Q90TAFXZC
(`T-NKMAKUC-REL-202505100505`): the module built by this repo loads and
runs, and reads live kernel memory.

## One-shot build

```sh
./build.sh                      # local (needs arm-none-eabi gcc/binutils)
# or
docker build --output out .     # fully reproducible, extracts out/research_kmem.ko
# or, with the Samsung-era toolchain (see below):
docker build --platform linux/amd64 -f Dockerfile.linaro --output out .
```

`build.sh` compiles `research_kmem.c` against the vendored kernel headers
(`kernel-headers/`, exactly the 241 files the compile touches, from the
Samsung 4.1.10 tree) and hand-links with `ld -r`. `verify.sh` then checks
the artifact against every wall below. The Docker image needs only Debian +
`gcc-arm-none-eabi` — nothing else.

### The ancient-toolchain variant (`Dockerfile.linaro`)

The TV kernel was built with `Tizen/Linaro GCC 6.2.1` (per its banner).
[Dockerfile.linaro](Dockerfile.linaro) builds the module with ARM's
`6-2017-q2-update` release (gcc 6.3.1, the same Linaro family) — verified
working on modern Debian; the result passes `verify.sh` and its reloc
profile is the same family as stock modules. Notes:

- The loader walls (PLT sections, vermagic, ARM mode, `-fno-unwind-tables`)
  are kernel conventions and are **not** removed by the old compiler — only
  the modern-gcc surprises are: the inline-asm section leak (wall 6, a
  gcc-16 behavior) and the DWARF5 issue.
- Linaro's own 6.2.1 archives are dead (`releases.linaro.org` redirects to
  a contact page); ARM's 6.3.1 CDN download is the closest survivor.
- The 2017 toolchain is x86_64-only: pass `--platform linux/amd64` on
  Apple Silicon (QEMU emulation, slower), native on amd64 Docker hosts.

## Why this recipe exists: the loader walls

Samsung's module loader enforces checks a normal module build fails one by
one. Each was hit and cleared on the TV, in loader order:

| # | Wall (TV error) | Fix |
|---|---|---|
| 1 | `No module found in object` | `__this_module` must exist in `.gnu.linkonce.this_module` (see source) |
| 2 | `version magic '%s' should be '%s'` | vermagic must be the TV's exact `0000000, release SMP preempt mod_unload ARMv7 p2v8 ` (`DTV_KERNEL_VERSION`, not the UTS release; trailing space matters) |
| 3 | `sections missing` | **`.core.plt` + `.init.plt` must exist** — Samsung's required-section check for the module-PLT sections. A stock `acmd.ko` carries exactly one zero byte in each (`SHF_WRITE\|SHF_ALLOC`); emitted by `plt.S` |
| 4 | `Unknown symbol in module` | compile `-fno-unwind-tables`, otherwise gcc emits `.ARM.exidx` referencing libgcc's `__aeabi_unwind_cpp_pr0/pr1` |
| 5 | `unknown relocation: 47` | compile in **ARM mode, not Thumb** — the loader refuses Thumb `MOVW`/`MOVT` relocs (types 47/48). Supported set = exactly what stock modules use: `R_ARM_ABS32` (2), `R_ARM_CALL` (28), `R_ARM_JUMP24` (29), ARM `MOVW_ABS_NC`/`MOVT_ABS` (43/44) |
| 6 | `out of bounds relocation, offset N size M` | **the PLT sections must be reloc-free** — any `.rel.core.plt`/`.rel.init.plt` entry is refused. Trap: an inline top-level asm corrupts GCC's section tracking and functions get emitted *into* `.core.plt`; hence the separate `plt.S` |
| 7 | mangled `dump_vas` list | kernel param parsing splits on commas — use `:` as the element separator |

Notes:

- **UEP gate:** with UEP down (the `s_uepStatus == 0` override), the module
  hook `sf_security_kernel_module_from_file` passes — modules load unsigned.
  With UEP up they are refused like any unsigned binary.
- The earlier lore that "ARM-mode modules are refused at interworking" was
  a misdiagnosis: stock modules carry hundreds of ARM-mode `R_ARM_CALL`s
  and load fine.
- An earlier claim that Samsung `.ko`s carry a `\n##UEP530…` signature
  trailer was retracted — the kernel image contains no such magic (likely
  an AI hallucination). The real gate is the section check above.

## Using it on the TV

```sh
# upload (no scp on the TV)
ssh root@TV 'cat > /opt/usr/share/tvroot/research_kmem.ko' < out/research_kmem.ko

# load (UEP must be down first — e.g. the loopback UEP firing)
ssh root@TV 'insmod /opt/usr/share/tvroot/research_kmem.ko enable=1'

# dump 16 bytes of live kernel memory at several VAs during init (the
# reliable read path) — colon-separated hex list, one load, no rmmod
ssh root@TV 'insmod /opt/usr/share/tvroot/research_kmem.ko enable=1 \
  dump_vas=0xc069342c:0xc01de10c; dmesg | grep dump@ | tail -2'
```

The module registers `/dev/kmem` (0600). **Do not `dd` from `/dev/kmem`:**
the file-*read* fop crashes the TV kernel (open/llseek/init are fine, and
`probe_kernel_read` from init is fine — the suspect is `copy_to_user` under
Tizen's SMACK uaccess; not yet root-caused). The TV recovers by itself
(reboot, and the persistent UEP chain re-fires), but use `dump_vas` instead.

**The TV cannot unload modules** — `rmmod` returns `Device or resource
busy` for every module on this kernel, so a loaded module stays loaded
until reboot. That's why `dump_vas` takes a *list*: everything is dumped
from one `insmod`. Also, script-*file* loading is UEP-refused even with
UEP down — always run scripts by content (`sh -c "$(cat file)"`).

## Verifying the UEP gates (the reason this module exists)

`verify-uep-gates.sh` runs on the TV and confirms the UEP gate patch is
live, byte-for-byte, from a single `insmod`:

```sh
# upload both files, then (fresh boot — the module must not be loaded yet):
ssh root@TV 'sh -c "$(cat /opt/usr/share/tvroot/verify-uep-gates.sh)" /opt/usr/share/tvroot/research_kmem.ko'
```

Field result (2026-10-26, Q90T with the loopback patch):

```
PASS c01de10c: 00 30 a0 e3 ...   ← gate 1: patched mov r3, #0
PASS c01de23c: 00 30 a0 e3 ...   ← gate 2: patched
PASS c01de524: 00 30 a0 e3 ...   ← gate 3: patched
PASS c069342c: 01 00 00 00 ...   ← s_uepStatus: untouched (RAM-only patch)
UEP GATES: all three patched, s_uepStatus untouched
```

## Files

- [research_kmem.c](research_kmem.c) — the module (source of truth for walls 1 and 3)
- [plt.S](plt.S) — the required `.core.plt`/`.init.plt` sections (wall 6)
- [build.sh](build.sh) — compile + hand-link with the field-verified flags
- [verify.sh](verify.sh) — local artifact verification (vermagic, sections, symbols, relocs)
- [verify-uep-gates.sh](verify-uep-gates.sh) — TV-side UEP gate check
- [kernel-headers/](kernel-headers/) — vendored Samsung 4.1.10 headers (exact compile closure)
- [Dockerfile](Dockerfile) — reproducible one-shot build (Debian gcc 12)
- [Dockerfile.linaro](Dockerfile.linaro) — the Samsung-era gcc 6.3.1 variant

## Related

- [gtalusan/samsung-q90t-tizen-root](https://github.com/gtalusan/samsung-q90t-tizen-root) —
  the Q90T research fork; see
  [docs/uep-bypass.md](https://github.com/gtalusan/samsung-q90t-tizen-root/blob/docs-uep-offset-note/docs/uep-bypass.md)
  §4 (the wall forensics) and
  [docs/methodology/04-kernel-recovery.md](https://github.com/gtalusan/samsung-q90t-tizen-root/blob/docs-uep-offset-note/docs/methodology/04-kernel-recovery.md)
  (the firmware map and the uImage header trap)
- The Q70T exchange: [cfloinc/samsung-q70t-tizen-root](https://github.com/cfloinc/samsung-q70t-tizen-root)
  [PR #1](https://github.com/cfloinc/samsung-q70t-tizen-root/pull/1) (the merge + Q&A) and
  [PR #2](https://github.com/cfloinc/samsung-q70t-tizen-root/pull/2) (the doc corrections)
