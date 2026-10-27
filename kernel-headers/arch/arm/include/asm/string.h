#ifndef __ASM_ARM_STRING_H
#define __ASM_ARM_STRING_H

/*
 * We don't do inline string functions, since the
 * optimised inline asm versions are not small.
 */

#define __HAVE_ARCH_STRRCHR
extern char * strrchr(const char * s, int c);

#define __HAVE_ARCH_STRCHR
extern char * strchr(const char * s, int c);

#define __HAVE_ARCH_MEMCPY
extern void *__memcpy(void *, const void *, __kernel_size_t);

#define memcpy(d, s, n) __memcpy((d), (s), (n))

#define __HAVE_ARCH_MEMMOVE
extern void *__memmove(void *, const void *, __kernel_size_t);

#define memmove(d, s, n) __memmove((d), (s), (n))

#define __HAVE_ARCH_MEMCHR
extern void * memchr(const void *, int, __kernel_size_t);

#define __HAVE_ARCH_MEMSET
extern void *__memset(void *, int, __kernel_size_t);
extern void __memzero(void *ptr, __kernel_size_t n);

#ifdef CONFIG_KASAN
	extern void *memset(void *, int, __kernel_size_t);
#else

#define memset(p,v,n)							\
	({								\
	 	void *__p = (p); size_t __n = n;			\
		if ((__n) != 0) {					\
			if (__builtin_constant_p((v)) && (v) == 0)	\
				__memzero((__p),(__n));			\
			else						\
				__memset((__p), (v), (__n));		\
		}							\
		(__p);							\
	})

#endif

#if defined(CONFIG_KASAN) && !defined(__SANITIZE_ADDRESS__)

/*
* For files that not instrumented (e.g. mm/slub.c) we
* should use not instrumented version of mem* functions.
*/

#undef memset

#define memset(p, v, n) __memset((p), (v), (n))
#define memcpy(d, s, n) __memcpy((d), (s), (n))
#define memmove(d, s, n) __memmove((d), (s), (n))

#endif

#endif
