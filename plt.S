/* Samsung's ARM module loader requires the module-PLT sections to exist
 * (a stock acmd.ko carries exactly one zero byte in each, flags
 * SHF_WRITE|SHF_ALLOC); without them insmod fails with "sections missing".
 *
 * Keep this in a separate .S: an inline top-level asm in the C file
 * corrupts GCC's section tracking and functions end up emitted INTO
 * .core.plt (with relocs the TV loader then refuses). */
	.section .core.plt, "aw", %progbits
	.byte 0
	.section .init.plt, "aw", %progbits
	.byte 0
