// SPDX-License-Identifier: GPL-2.0-only OR AGPL-3.0-only
/*
 * research_kmem — the minimal loadable kernel module for Samsung Tizen 5.5
 * TVs (Linux 4.1.10, T-NKMAKUC family, ARMv7).
 *
 * Field-verified 2026-10-26 on a QN55Q90TAFXZC: this exact source builds a
 * module the TV's loader accepts, and it loads and runs with UEP down.
 * See README.md for the five loader walls this file (plus build.sh) clears.
 *
 * What it does:
 *  - registers /dev/kmem (0600) backed by probe_kernel_read/probe_kernel_write
 *  - at init, if dump_vas is set, prints 16 bytes of live kernel memory for
 *    each VA in the colon-separated hex list (colons, because kernel
 *    param parsing splits on commas) (the reliable read path — see
 *    the caveat below; the TV cannot unload modules, so one load does all)
 *
 * CAVEAT (observed on the TV): the file *read* fop crashes the TV kernel
 * (suspect: copy_to_user under Tizen's SMACK uaccess). open/llseek/init are
 * fine, and probe_kernel_read from init is fine. Use dump_va for reads.
 */
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/* Exported by the target ARMv7 kernel. */
extern void v7_coherent_kern_range(unsigned long start, unsigned long end);

static bool enable;
static unsigned long test_addr;
static char *dump_vas;
static char test_buffer[16] = "KMEM-ORIGINAL";
module_param(enable, bool, 0400);
module_param(test_addr, ulong, 0444);
module_param(dump_vas, charp, 0444);
MODULE_PARM_DESC(enable, "Enable dangerous raw kernel-memory access");
MODULE_PARM_DESC(test_addr, "Writable module-buffer address for self-test");
MODULE_PARM_DESC(dump_vas, "Colon-separated hex VAs: dump 16 bytes of each during init");

/* Dependency-free hex parser (no string-lib exports needed). Accepts the
 * optional 0x/0X prefix on each element. */
static unsigned long parse_hex(const char **sp)
{
	const char *s = *sp;
	unsigned long v = 0;
	int pass;

	while (*s == ' ' || *s == ',' || *s == ':' || *s == '\t')
		s++;
	for (pass = 0; pass < 2; pass++) {
		while (1) {
			char c = *s;
			unsigned d;

			if (c >= '0' && c <= '9')
				d = c - '0';
			else if (c >= 'a' && c <= 'f')
				d = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				d = c - 'A' + 10;
			else
				break;
			v = (v << 4) | d;
			s++;
		}
		if (!((*s == 'x' || *s == 'X') && v == 0))
			break;
		s++;	/* 0x prefix: skip the marker and parse the digits */
	}
	*sp = s;
	return v;
}

static void kmem_dump(unsigned long va)
{
	char buf[16] = {0};
	long r = probe_kernel_read(buf, (const void *)va, 16);

	pr_info("research_kmem: dump@0x%lx rc=%ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		va, r,
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
		buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
}

static int research_kmem_open(struct inode *inode, struct file *file)
{
	if (!enable || !capable(CAP_SYS_RAWIO))
		return -EPERM;
	return 0;
}

static loff_t research_kmem_llseek(struct file *file, loff_t off, int whence)
{
	loff_t pos;

	switch (whence) {
	case SEEK_SET:
		pos = off;
		break;
	case SEEK_CUR:
		pos = file->f_pos + off;
		break;
	default:
		return -EINVAL;
	}
	if (pos < 0)
		return -EINVAL;
	file->f_pos = pos;
	return pos;
}

static ssize_t research_kmem_read(struct file *file, char __user *buf,
				  size_t len, loff_t *ppos)
{
	char *tmp;
	size_t done = 0;
	int ret = 0;

	if (!enable || !capable(CAP_SYS_RAWIO))
		return -EPERM;
	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	while (done < len) {
		size_t n = min_t(size_t, PAGE_SIZE, len - done);
		unsigned long addr = (unsigned long)(*ppos + done);

		ret = probe_kernel_read(tmp, (const void *)addr, n);
		if (ret)
			break;
		if (copy_to_user(buf + done, tmp, n)) {
			ret = -EFAULT;
			break;
		}
		done += n;
	}
	kfree(tmp);
	if (done) {
		*ppos += done;
		return done;
	}
	return ret;
}

static ssize_t research_kmem_write(struct file *file, const char __user *buf,
				   size_t len, loff_t *ppos)
{
	char *tmp;
	size_t done = 0;
	int ret = 0;

	if (!enable || !capable(CAP_SYS_RAWIO))
		return -EPERM;
	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	while (done < len) {
		size_t n = min_t(size_t, PAGE_SIZE, len - done);
		unsigned long addr = (unsigned long)(*ppos + done);

		if (copy_from_user(tmp, buf + done, n)) {
			ret = -EFAULT;
			break;
		}
		ret = probe_kernel_write((void *)addr, tmp, n);
		if (ret)
			break;
		v7_coherent_kern_range(addr, addr + n);
		done += n;
	}
	kfree(tmp);
	if (done) {
		*ppos += done;
		return done;
	}
	return ret;
}

static const struct file_operations research_kmem_fops = {
	.owner = THIS_MODULE,
	.open = research_kmem_open,
	.llseek = research_kmem_llseek,
	.read = research_kmem_read,
	.write = research_kmem_write,
};

static struct miscdevice research_kmem_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "kmem",
	.mode = 0600,
	.fops = &research_kmem_fops,
};

static int __init research_kmem_init(void)
{
	if (!enable)
		return -EPERM;
	test_addr = (unsigned long)test_buffer;
	pr_info("research_kmem: self-test buffer at 0x%lx\n", test_addr);
	if (dump_vas && dump_vas[0]) {
		const char *s = dump_vas;

		while (*s) {
			unsigned long va = parse_hex(&s);

			if (va)
				kmem_dump(va);
			if (!*s)
				break;
			if (*s != ':' && *s != ',') {
				pr_info("research_kmem: bad dump_vas at '%s'\n", s);
				break;
			}
		}
	}
	return misc_register(&research_kmem_dev);
}

static void __exit research_kmem_exit(void)
{
	misc_deregister(&research_kmem_dev);
}

module_init(research_kmem_init);
module_exit(research_kmem_exit);
MODULE_LICENSE("GPL");

MODULE_INFO(vermagic, "0000000, release SMP preempt mod_unload ARMv7 p2v8 ");
MODULE_INFO(name, KBUILD_MODNAME);
__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};
MODULE_DESCRIPTION("Opt-in research /dev/kmem for Samsung TV lab");
MODULE_LICENSE("GPL");
