// File created: 2011-08-09 19:22:21

#include "cell.both.h"

#include <assert.h>

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define GNU_X86_ASM 1
#else
#define GNU_X86_ASM 0
#endif

#if !MUSHSPACE_93
static mushucell mushucell_mod_inv(mushucell);
#endif

mushcell mushcell_max(mushcell a, mushcell b) { return a > b ? a : b; }
mushcell mushcell_min(mushcell a, mushcell b) { return a < b ? a : b; }

void mushcell_max_into(mushcell* a, mushcell b) { *a = mushcell_max(*a, b); }
void mushcell_min_into(mushcell* a, mushcell b) { *a = mushcell_min(*a, b); }

mushcell mushcell_add(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a + (mushucell)b);
}
mushcell mushcell_sub(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a - (mushucell)b);
}
mushcell mushcell_mul(mushcell a, mushcell b) {
	return (mushcell)((mushucell)a * (mushucell)b);
}
mushcell mushcell_inc(mushcell n) { return mushcell_add(n, 1); }
mushcell mushcell_dec(mushcell n) { return mushcell_sub(n, 1); }
void mushcell_add_into(mushcell* a, mushcell b) { *a = mushcell_add(*a, b); }
void mushcell_sub_into(mushcell* a, mushcell b) { *a = mushcell_sub(*a, b); }

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

#if !MUSHSPACE_93
bool mushucell_mod_div(mushucell a, mushucell b, mushucell* x,
                       uint_fast8_t* gcd_lg)
{
	assert (a != 0);

	// mod_inv can't deal with even numbers, so handle that here.
	*gcd_lg = 0;
	while (a%2 == 0 && b%2 == 0) {
		a /= 2;
		b /= 2;
		++*gcd_lg;
	}

	// If a is even and b odd then no solution exists.
	if (a%2 == 0)
		return false;

	*x = mushucell_mod_inv(a) * b;
	return true;
}

// Solves for x in the equation ax = 1 (mod 2^(sizeof(mushucell) * 8)), given
// a. Alternatively stated, finds the modular inverse of a in the same ring as
// mushucell's normal integer arithmetic works.
//
// For odd values of a, it holds that a * modInv(a) = 1.
//
// For even values, this asserts: the inverse exists only if a is coprime with
// the modulus.
//
// For simplicity, the comments speak of 32-bit throughout but this works
// regardless of the size of mushucell.
static mushucell mushucell_mod_inv(mushucell a) {
	assert (a%2 != 0);

	// We use the Extended Euclidean algorithm, with a few tricks at the start
	// to deal with the fact that mushucell can't represent the initial modulus.

	// We need quot = floor(2^32 / a).
	//
	// floor(2^31 / a) * 2 differs from floor(2^32 / a) by at most 1. I seem
	// unable to discern what property a needs to have for them to differ, so we
	// figure it out using a possibly suboptimal method.
	mushucell gcd = (mushucell)1 << (sizeof(mushucell)*8 - 1);
	mushucell quot;

	if (a <= gcd)
		quot = gcd / a * 2;
	else {
		// The above algorithm obviously doesn't work if a exceeds gcd:
		// fortunately, we know that quot = 1 in all those cases.
		quot = 1;
	}

	// So now quot is either floor(2^32 / a) or floor(2^32 / a) - 1.
	//
	// 2^32 = quot * a + rem
	//
	// If quot is the former, then rem = -a * quot. Otherwise, rem = -a * (1 +
	// quot) and quot needs to be corrected.
	//
	// So we try the former case. For this to be the correct remainder, it
	// should be in the range [0,a). If it isn't, we know that quot is off by
	// one.
	mushucell rem = -a * quot;
	if (rem >= a) {
		rem -= a;
		++quot;
	}

	// And now we can continue using normal division.
	//
	// We peeled only half of the first iteration above so the loop condition is
	// in the middle.
	mushucell x = 0;
	for (mushucell u = 1;;) {
		mushucell old_x = x;

		gcd = a;
		a = rem;
		x = u;
		u = old_x - u*quot;

		if (!a)
			break;

		quot = gcd / a;
		rem  = gcd % a;
	}
	assert (x != 0);
	return x;
}
#endif // !MUSHSPACE_93
