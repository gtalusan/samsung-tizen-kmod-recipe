#ifndef _LINUX_KASAN_H
#define _LINUX_KASAN_H

#include <linux/types.h>

struct kmem_cache;
struct page;
struct vm_struct;

#ifdef CONFIG_KASAN

#define KASAN_SHADOW_SCALE_SHIFT 3
#ifdef CONFIG_KASAN_SHADOW_OFFSET
#define KASAN_SHADOW_OFFSET _AC(CONFIG_KASAN_SHADOW_OFFSET, UL)
#else
extern unsigned long __read_mostly kasan_shadow_offset;
#define KASAN_SHADOW_OFFSET (kasan_shadow_offset)
#endif

#include <asm/kasan.h>
#include <linux/sched.h>
#include <linux/mm.h>

#ifndef CONFIG_KASAN_SHADOW_OFFSET
/* Reserves shadow memory. */
extern unsigned long kasan_vmalloc_shadow_start;
void kasan_alloc_shadow(void);
void kasan_init_shadow(void);
#else
static inline void kasan_init_shadow(void) {}
static inline void kasan_alloc_shadow(void) {}
#endif

static inline void *kasan_mem_to_shadow(const void *addr)
{
#ifdef CONFIG_KASAN_VMALLOC
	if (is_vmalloc_addr(addr)) {
		return (void *)((((unsigned long)addr - VMALLOC_START) >> PAGE_SHIFT) +
			kasan_vmalloc_shadow_start);
	}
#endif

	return (void *)((unsigned long)addr >> KASAN_SHADOW_SCALE_SHIFT)
		+ KASAN_SHADOW_OFFSET;
}

/* Enable reporting bugs after kasan_disable_current() */
static inline void kasan_enable_current(void)
{
	current->kasan_depth++;
}

/* Disable reporting bugs for current task */
static inline void kasan_disable_current(void)
{
	current->kasan_depth--;
}

void kasan_unpoison_shadow(const void *address, size_t size);

void kasan_alloc_pages(struct page *page, unsigned int order);
void kasan_free_pages(struct page *page, unsigned int order);

void kasan_poison_slab(struct page *page);
void kasan_unpoison_object_data(struct kmem_cache *cache, void *object);
void kasan_poison_object_data(struct kmem_cache *cache, void *object);

void kasan_kmalloc_large(const void *ptr, size_t size);
void kasan_kfree_large(const void *ptr);
void kasan_kfree(void *ptr);
void kasan_kmalloc(struct kmem_cache *s, const void *object, size_t size);
void kasan_krealloc(const void *object, size_t new_size);

void kasan_slab_alloc(struct kmem_cache *s, void *object);
void kasan_slab_free(struct kmem_cache *s, void *object);

int kasan_module_alloc(void *addr, size_t size);
void kasan_free_shadow(const struct vm_struct *vm);

#ifdef CONFIG_KASAN_GLOBALS
void kasan_module_load(void *mod, size_t size);
#else
static inline void kasan_module_load(void *mod, size_t size) {}
#endif

#else /* CONFIG_KASAN */

static inline void kasan_init_shadow(void) {}
static inline void kasan_alloc_shadow(void) {}

static inline void kasan_unpoison_shadow(const void *address, size_t size) {}

static inline void kasan_enable_current(void) {}
static inline void kasan_disable_current(void) {}

static inline void kasan_alloc_pages(struct page *page, unsigned int order) {}
static inline void kasan_free_pages(struct page *page, unsigned int order) {}

static inline void kasan_poison_slab(struct page *page) {}
static inline void kasan_unpoison_object_data(struct kmem_cache *cache,
					void *object) {}
static inline void kasan_poison_object_data(struct kmem_cache *cache,
					void *object) {}

static inline void kasan_kmalloc_large(void *ptr, size_t size) {}
static inline void kasan_kfree_large(const void *ptr) {}
static inline void kasan_kfree(void *ptr) {}
static inline void kasan_kmalloc(struct kmem_cache *s, const void *object,
				size_t size) {}
static inline void kasan_krealloc(const void *object, size_t new_size) {}

static inline void kasan_slab_alloc(struct kmem_cache *s, void *object) {}
static inline void kasan_slab_free(struct kmem_cache *s, void *object) {}

static inline int kasan_module_alloc(void *addr, size_t size) { return 0; }
static inline void kasan_free_shadow(const struct vm_struct *vm) {}

#endif /* CONFIG_KASAN */

#ifdef CONFIG_KASAN_VMALLOC

void kasan_vmalloc_nopageguard(unsigned long addr, size_t size);
void kasan_vmalloc(unsigned long addr, size_t size);
void kasan_vfree(unsigned long addr, size_t size);

#else /* CONFIG_KASAN_VMALLOC */

static inline void kasan_vmalloc_nopageguard(unsigned long addr, size_t size) {}
static inline void kasan_vmalloc(unsigned long addr, size_t size) {}
static inline void kasan_vfree(unsigned long addr, size_t size) {}

#endif /* CONFIG_KASAN_VMALLOC */

#endif /* LINUX_KASAN_H */
