/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractivechaos@aol.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
  An example:

#include "khash.h"
KHASH_MAP_INIT_INT(32, char)
int main() {
	int ret, is_missing;
	khiter_t k;
	khash_t(32) *h = kh_init(32);
	k = kh_put(32, h, 5, &ret);
	if (!ret) kh_del(32, h, k);
	kh_value(h, k) = 10;
	k = kh_get(32, h, 10);
	is_missing = (k == kh_end(h));
	k = kh_get(32, h, 5);
	kh_del(32, h, k);
	for (k = kh_begin(h); k != kh_end(h); ++k)
		if (kh_exist(h, k)) kh_value(h, k) = 1;
	kh_destroy(32, h);
	return 0;
}
*/

/*
  2008-09-19 (0.2.3):

	* Corrected the example
	* Improved interfaces

  2008-09-11 (0.2.2):

	* Improved speed a little in kh_put()

  2008-09-10 (0.2.1):

	* Added kh_clear()
	* Fixed a compiling error

  2008-09-02 (0.2.0):

	* Changed to token concatenation which increases flexibility.

  2008-08-31 (0.1.2):

	* Fixed a bug in kh_get(), which has not been tested previously.

  2008-08-31 (0.1.1):

	* Added destructor
*/


#ifndef __AC_KHASH_H
#define __AC_KHASH_H

#define AC_VERSION_KHASH_H "0.2.2"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AC_CATH(A,B) A##B
#define AC_CAT(A,B) AC_CATH(A,B)

typedef size_t khint_t, khiter_t;

#if SIZE_MAX <= 0xffffffff
#define __ac_HASH_PRIME_SIZE 32
static const uint32_t __ac_prime_list[__ac_HASH_PRIME_SIZE] =
{
  0ul,          3ul,          11ul,         23ul,         53ul,
  97ul,         193ul,        389ul,        769ul,        1543ul,
  3079ul,       6151ul,       12289ul,      24593ul,      49157ul,
  98317ul,      196613ul,     393241ul,     786433ul,     1572869ul,
  3145739ul,    6291469ul,    12582917ul,   25165843ul,   50331653ul,
  100663319ul,  201326611ul,  402653189ul,  805306457ul,  1610612741ul,
  3221225473ul, 4294967291ul
};
#elif SIZE_MAX <= 0xffffffffffffffff
#define __ac_HASH_PRIME_SIZE 64
static const uint64_t __ac_prime_list[__ac_HASH_PRIME_SIZE] =
{
  0ull,                   3ull,                   11ull,
  23ull,                  53ull,                  97ull,
  193ull,                 389ull,                 769ull,
  1543ull,                3079ull,                6151ull,
  12289ull,               24593ull,               49157ull,
  98317ull,               196613ull,              393241ull,
  786433ull,              1572869ull,             3145739ull,
  6291469ull,             12582917ull,            25165843ull,
  50331653ull,            100663319ull,           201326611ull,
  402653189ull,           805306457ull,           1610612741ull,
  3221225473ull,          6442450939ull,          12884901877ull,
  25769803751ull,         51539607503ull,         103079215007ull,
  206158430017ull,        412316860027ull,        824633720047ull,
  1649267440087ull,       3298534880167ull,       6597069760331ull,
  13194139520663ull,      26388279041353ull,      52776558082697ull,
  105553116165401ull,     211106232330821ull,     422212464661667ull,
  844424929323337ull,     1688849858646677ull,    3377699717293357ull,
  6755399434586741ull,    13510798869173477ull,   27021597738346961ull,
  54043195476693911ull,   108086390953387817ull,  216172781906775641ull,
  432345563813551279ull,  864691127627102579ull,  1729382255254205147ull,
  3458764510508410273ull, 6917529021016820543ull, 13835058042033641131ull,
  18446744073709551557ull
};
#else
#error No prime numbers bigger than 1<<64-1 exist!
#endif

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))

static const double __ac_HASH_UPPER = 0.77;

#define khash_t(name) AC_CAT(AC_CAT(kh_,name),_t)

#define KHASH_INIT(name, khkey_t, khval_t, kh_is_map, __hash_func, __hash_equal) \
	typedef struct {                                                     \
		khint_t n_buckets, size, n_occupied, upper_bound;                 \
		uint32_t *flags;                                                  \
		khkey_t *keys;                                                    \
		khval_t *vals;                                                    \
	} khash_t(name);                                                     \
	static inline khash_t(name) *AC_CAT(kh_init_,name)() {               \
		return calloc(1, sizeof(khash_t(name)));                          \
	}                                                                    \
	static inline void AC_CAT(kh_destroy_,name)(khash_t(name) *h)        \
	{                                                                    \
		if (h) {                                                          \
			free(h->keys); free(h->flags);                                 \
			free(h->vals);                                                 \
			free(h);                                                       \
		}                                                                 \
	}                                                                    \
	static inline void AC_CAT(kh_clear_,name)(khash_t(name) *h)          \
	{                                                                    \
		if (h && h->flags) {                                              \
			memset(h->flags, 0xaa, ((h->n_buckets>>4) + 1) * sizeof(uint32_t)); \
			h->size = h->n_occupied = 0;                                   \
		}                                                                 \
	}                                                                    \
	static inline khint_t AC_CAT(kh_get_,name)(const khash_t(name) *h, khkey_t key) \
	{                                                                    \
		if (h->n_buckets) {                                               \
			khint_t inc, k, i, last;                                       \
			k = __hash_func(key); i = k % h->n_buckets;                    \
			inc = 1 + k % (h->n_buckets - 1); last = i;                    \
			while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
				if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets;    \
				else i += inc;                                              \
				if (i == last) return h->n_buckets;                         \
			}                                                              \
			return __ac_iseither(h->flags, i)? h->n_buckets : i;           \
		} else return 0;                                                  \
	}                                                                    \
	static inline void AC_CAT(kh_resize_,name)(khash_t(name) *h, khint_t new_n_buckets) \
	{                                                                    \
		uint32_t *new_flags = 0;                                          \
		khint_t j = 1;                                                    \
		{                                                                 \
			khint_t t = __ac_HASH_PRIME_SIZE - 1;                          \
			while (__ac_prime_list[t] > new_n_buckets) --t;                \
			new_n_buckets = __ac_prime_list[t+1];                          \
			if (h->size >= (khint_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0; \
			else {                                                         \
				new_flags = (uint32_t*)malloc(((new_n_buckets>>4) + 1) * sizeof(uint32_t)); \
				memset(new_flags, 0xaa, ((new_n_buckets>>4) + 1) * sizeof(uint32_t)); \
				if (h->n_buckets < new_n_buckets) {                         \
					h->keys = (khkey_t*)realloc(h->keys, new_n_buckets * sizeof(khkey_t)); \
					if (kh_is_map)                                           \
						h->vals = (khval_t*)realloc(h->vals, new_n_buckets * sizeof(khval_t)); \
				}                                                           \
			}                                                              \
		}                                                                 \
		if (j) {                                                          \
			for (j = 0; j != h->n_buckets; ++j) {                          \
				if (__ac_iseither(h->flags, j) == 0) {                      \
					khkey_t key = h->keys[j];                                \
					khval_t val;                                             \
					if (kh_is_map) val = h->vals[j];                         \
					__ac_set_isdel_true(h->flags, j);                        \
					while (1) {                                              \
						khint_t inc, k, i;                                    \
						k = __hash_func(key);                                 \
						i = k % new_n_buckets;                                \
						inc = 1 + k % (new_n_buckets - 1);                    \
						while (!__ac_isempty(new_flags, i)) {                 \
							if (i + inc >= new_n_buckets) i = i + inc - new_n_buckets; \
							else i += inc;                                     \
						}                                                     \
						__ac_set_isempty_false(new_flags, i);                 \
						if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) { \
							{ khkey_t tmp = h->keys[i]; h->keys[i] = key; key = tmp; } \
							if (kh_is_map) { khval_t tmp = h->vals[i]; h->vals[i] = val; val = tmp; } \
							__ac_set_isdel_true(h->flags, i);                  \
						} else {                                              \
							h->keys[i] = key;                                  \
							if (kh_is_map) h->vals[i] = val;                   \
							break;                                             \
						}                                                     \
					}                                                        \
				}                                                           \
			}                                                              \
			if (h->n_buckets > new_n_buckets) {                            \
				h->keys = (khkey_t*)realloc(h->keys, new_n_buckets * sizeof(khkey_t)); \
				if (kh_is_map)                                              \
					h->vals = (khval_t*)realloc(h->vals, new_n_buckets * sizeof(khval_t)); \
			}                                                              \
			free(h->flags);                                                \
			h->flags = new_flags;                                          \
			h->n_buckets = new_n_buckets;                                  \
			h->n_occupied = h->size;                                       \
			h->upper_bound = (khint_t)(h->n_buckets * __ac_HASH_UPPER + 0.5); \
		}                                                                 \
	}                                                                    \
	static inline khint_t AC_CAT(kh_put_,name)(khash_t(name) *h, khkey_t key, int *ret) \
	{                                                                    \
		khint_t x;                                                        \
		if (h->n_occupied >= h->upper_bound) {                            \
			if (h->n_buckets > (h->size<<1)) AC_CAT(kh_resize_,name)(h, h->n_buckets - 1); \
			else AC_CAT(kh_resize_,name)(h, h->n_buckets + 1);             \
		}                                                                 \
		{                                                                 \
			khint_t inc, k, i, site, last;                                 \
			x = site = h->n_buckets; k = __hash_func(key); i = k % h->n_buckets; \
			if (__ac_isempty(h->flags, i)) x = i;                          \
			else {                                                         \
				inc = 1 + k % (h->n_buckets - 1); last = i;                 \
				while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
					if (__ac_isdel(h->flags, i)) site = i;                   \
					if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets; \
					else i += inc;                                           \
					if (i == last) { x = site; break; }                      \
				}                                                           \
				if (x == h->n_buckets) {                                    \
					if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site; \
					else x = i;                                              \
				}                                                           \
			}                                                              \
		}                                                                 \
		if (__ac_isempty(h->flags, x)) {                                  \
			h->keys[x] = key;                                              \
			__ac_set_isboth_false(h->flags, x);                            \
			++h->size; ++h->n_occupied;                                    \
			*ret = 1;                                                      \
		} else if (__ac_isdel(h->flags, x)) {                             \
			h->keys[x] = key;                                              \
			__ac_set_isboth_false(h->flags, x);                            \
			++h->size;                                                     \
			*ret = 2;                                                      \
		} else *ret = 0;                                                  \
		return x;                                                         \
	}                                                                    \
	static inline void AC_CAT(kh_del_,name)(khash_t(name) *h, khint_t x) \
	{                                                                    \
		if (x != h->n_buckets && !__ac_iseither(h->flags, x)) {           \
			__ac_set_isdel_true(h->flags, x);                              \
			--h->size;                                                     \
		}                                                                 \
	}

/* --- BEGIN OF HASH FUNCTIONS --- */

#define kh_int_hash_func(key) (uint32_t)(key)
#define kh_int_hash_equal(a, b) (a == b)
#define kh_int64_hash_func(key) (uint32_t)((key)>>33^(key)^(key)<<11)
#define kh_int64_hash_equal(a, b) (a == b)
static inline khint_t __ac_X31_hash_string(const char *s)
{
	khint_t h = *s;
	if (h) for (++s ; *s; ++s) h = (h << 5) - h + *s;
	return h;
}
#define kh_str_hash_func(key) __ac_X31_hash_string(key)
#define kh_str_hash_equal(a, b) (strcmp(a, b) == 0)

/* --- END OF HASH FUNCTIONS --- */

/* Other necessary macros... */

#define kh_init(name) AC_CAT(kh_init_,name)()
#define kh_destroy(name, h) AC_CAT(kh_destroy_,name)(h)
#define kh_clear(name, h) AC_CAT(kh_clear_,name)(h)
#define kh_resize(name, h, s) AC_CAT(kh_resize_,name)(h, s)
#define kh_put(name, h, k, r) AC_CAT(kh_put_,name)(h, k, r)
#define kh_get(name, h, k) AC_CAT(kh_get_,name)(h, k)
#define kh_del(name, h, k) AC_CAT(kh_del_,name)(h, k)

#define kh_exist(h, x) (!__ac_iseither((h)->flags, (x)))
#define kh_key(h, x) ((h)->keys[x])
#define kh_val(h, x) ((h)->vals[x])
#define kh_value(h, x) ((h)->vals[x])
#define kh_begin(h) (khint_t)(0)
#define kh_end(h) ((h)->n_buckets)
#define kh_size(h) ((h)->size)
#define kh_n_buckets(h) ((h)->n_buckets)

/* More conenient interfaces */

#define KHASH_SET_INIT_INT(name) \
	KHASH_INIT(name, uint32_t, char, 0, kh_int_hash_func, kh_int_hash_equal)

#define KHASH_MAP_INIT_INT(name, khval_t) \
	KHASH_INIT(name, uint32_t, khval_t, 1, kh_int_hash_func, kh_int_hash_equal)

#define KHASH_SET_INIT_INT64(name) \
	KHASH_INIT(name, uint64_t, char, 0, kh_int64_hash_func, kh_int64_hash_equal)

#define KHASH_MAP_INIT_INT64(name, khval_t) \
	KHASH_INIT(name, uint64_t, khval_t, 1, kh_int64_hash_func, kh_int64_hash_equal)

typedef const char *kh_cstr_t;
#define KHASH_SET_INIT_STR(name) \
	KHASH_INIT(name, kh_cstr_t, char, 0, kh_str_hash_func, kh_str_hash_equal)

#define KHASH_MAP_INIT_STR(name, khval_t) \
	KHASH_INIT(name, kh_cstr_t, khval_t, 1, kh_str_hash_func, kh_str_hash_equal)

#endif /* __AC_KHASH_H */
