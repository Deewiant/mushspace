// File created: 2011-08-09 19:22:21

#include "cell.both.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define GNU_X86_ASM 1
#else
#define GNU_X86_ASM 0
#endif

mushcell mushcell_max(mushcell a, mushcell b) { return a > b ? a : b; }
mushcell mushcell_min(mushcell a, mushcell b) { return a < b ? a : b; }

mushcell mushcell_add(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a + (mushucell)b);
}
mushcell mushcell_sub(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a - (mushucell)b);
}
mushcell mushcell_inc(mushcell n) {
	return mushcell_add(n, 1);
}

#if !MUSHSPACE_93
mushcell mushcell_add_clamped(mushcell a, mushcell b) {
	mushcell result;

#if GNU_X86_ASM && MUSHCELL_MAX < 0xffffffff

	__asm(  "addl %2,%1; movl %3,%0; cmovnol %1,%0"
	      : "=r"(result), "+r"(a)
	      : "r"(b), "n"(MUSHCELL_MAX));

#elif GNU_X86_ASM && MUSHCELL_MAX < 0xffffffffffffffff

	__asm(  "addq %2,%1; movq %3,%0; cmovnoq %1,%0"
	      : "=r"(result), "+r"(a)
	      : "r"(b), "n"(MUSHCELL_MAX));

#else
	result = a > MUSHCELL_MAX - b ? MUSHCELL_MAX : a + b;
#endif
	return result;
}
mushcell mushcell_sub_clamped(mushcell a, mushcell b) {
	mushcell result;

#if GNU_X86_ASM && MUSHCELL_MAX < 0xffffffff

	__asm(  "subl %2,%1; movl %3,%0; cmovnol %1,%0"
	      : "=r"(result), "+r"(a)
	      : "r"(b), "n"(MUSHCELL_MIN));

#elif GNU_X86_ASM && MUSHCELL_MAX < 0xffffffffffffffff

	__asm(  "subq %2,%1; movq %3,%0; cmovnoq %1,%0"
	      : "=r"(result), "+r"(a)
	      : "r"(b), "n"(MUSHCELL_MIN));

#else
	result = a < MUSHCELL_MIN + b ? MUSHCELL_MIN : a + b;
#endif
	return result;
}
#endif

void mushcell_space(mushcell* p, size_t l) {
	for (mushcell *e = p + l; p != e; ++p)
		*p = ' ';
}
