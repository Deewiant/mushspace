// File created: 2011-08-07 17:31:02

#include "bakaabb.98.h"

#include <assert.h>
#include <stdint.h>

#include "lib/khash.h"

struct mushbakaabb_iter { khiter_t k; };

const size_t mushbakaabb_iter_sizeof = sizeof(mushbakaabb_iter);

static size_t hash(mushcoords);

KHASH_INIT(mushcoords, mushcoords, mushcell, true, hash, mushcoords_equal)

bool mushbakaabb_init(mushbakaabb* bak, mushcoords c) {
   bak->data = kh_init(mushcoords);
   bak->bounds = (mushbounds){c,c};
   return bak->data != NULL;
}
void mushbakaabb_free(mushbakaabb* bak) {
   kh_destroy(mushcoords, bak->data);
}
bool mushbakaabb_copy(mushbakaabb* copy, const mushbakaabb* bak) {
   if (!kh_copy(mushcoords, copy->data, bak->data))
      return false;
   copy->bounds = bak->bounds;
   return true;
}

mushcell mushbakaabb_get(const mushbakaabb* bak, mushcoords c) {
   const khash_t(mushcoords) *hash = bak->data;
   khint_t i = kh_get(mushcoords, hash, c);
   return i == kh_end(hash) ? ' ' : kh_value(hash, i);
}
mushcell* mushbakaabb_get_ptr_unsafe(mushbakaabb* bak, mushcoords c) {
   const khash_t(mushcoords) *hash = bak->data;
   return &kh_value(hash, kh_get(mushcoords, hash, c));
}

bool mushbakaabb_put(mushbakaabb* bak, mushcoords p, mushcell c) {
   khash_t(mushcoords) *hash = bak->data;

   if (c == ' ') {
      khint_t i = kh_get(mushcoords, hash, p);
      if (i != kh_end(hash))
         kh_del(mushcoords, hash, i);
      return true;
   }

   int status;
   khint_t i = kh_put(mushcoords, hash, p, &status);
   if (status == -1)
      return false;
   kh_value(hash, i) = c;

   mushbounds_expand_to_cover(&bak->bounds, &(mushbounds){p,p});
   return true;
}

size_t mushbakaabb_size(const mushbakaabb* bak) {
   const khash_t(mushcoords) *hash = bak->data;
   return kh_size(hash);
}

mushbakaabb_iter* mushbakaabb_it_start(const mushbakaabb* bak, void* vp) {
   mushbakaabb_iter* it = vp ? vp : malloc(sizeof *it);
   if (it) {
      const khash_t(mushcoords) *hash = bak->data;
      *it = (mushbakaabb_iter){ kh_begin(hash) };
   }
   return it;
}
void mushbakaabb_it_stop(mushbakaabb_iter* it) { (void)it; }

bool mushbakaabb_it_done(const mushbakaabb_iter* it, const mushbakaabb* bak)
{
   const khash_t(mushcoords) *hash = bak->data;
   return it->k != kh_end(hash);
}

void mushbakaabb_it_next(mushbakaabb_iter* it, const mushbakaabb* bak) {
   (void)bak;
   ++it->k;
}
void mushbakaabb_it_remove(mushbakaabb_iter* it, mushbakaabb* bak) {
   khash_t(mushcoords) *hash = bak->data;
   kh_del(mushcoords, hash, it->k);
}

mushcoords mushbakaabb_it_pos(
   const mushbakaabb_iter* it, const mushbakaabb* bak)
{
   const khash_t(mushcoords) *hash = bak->data;
   return kh_key(hash, it->k);
}
mushcell mushbakaabb_it_val(const mushbakaabb_iter* it, const mushbakaabb* bak)
{
   const khash_t(mushcoords) *hash = bak->data;
   return kh_value(hash, it->k);
}

static size_t hash(mushcoords c) {
#if SIZE_MAX == 0xffffffff
   // MurmurHash3_x86_32
   uint32_t h = 0;

   assert ((sizeof c) % 4 == 0);

   const uint8_t *data = (const uint8_t*)&c;
   static const int nblocks = (sizeof c) / 4;

   const uint32_t *blocks = (const uint32_t*)(data + nblocks*4);

   for (int i = -nblocks; i; ++i) {
      uint32_t k = blocks[i];

      k *= 0xcc9e2d51;
      k = k << 15 | k >> (32-15);
      k *= 0x1b873593;

      h ^= k;
      h = h << 13 | h >> (32-13);
      h = h*5 + 0xe6546b64;
   }

   h ^= sizeof c;
   h ^= h >> 16;
   h *= 0x85ebca6b;
   h ^= h >> 13;
   h *= 0xc2b2ae35;
   h ^= h >> 16;

   return h;
#elif SIZE_MAX == 0xffffffffffffffff
   // The first 64-bit half of MurmurHash3_x64_128
   uint64_t h1 = 0, h2 = 0;

   assert ((sizeof c) % 4 == 0);

   const uint8_t *data = (const uint8_t*)&c;
   static const int nblocks = (sizeof c) / 16;

   static const uint64_t c1 = 0x87c37b91114253d5ULL,
                         c2 = 0x4cf5ad432745937fULL;

   const uint64_t *blocks = (const uint64_t*)data;

   for (int i = 0; i < nblocks; ++i) {
      uint64_t k1 = blocks[2*i],
               k2 = blocks[2*i + 1];

      k1 *= c1; k1 = k1 << 31 | k1 >> 33; k1 *= c2; h1 ^= k1;
      h1 = h1 << 27 | h1 >> 37; h1 += h2; h1 = 5*h1 + 0x52dce729;
      k2 *= c2; k2 = k2 << 33 | k2 >> 31; k2 *= c1; h2 ^= k2;
      h2 = h2 << 31 | h2 >> 33; h2 += h1; h2 = 5*h2 + 0x38495ab5;
   }

   const uint8_t *tail = data + nblocks*16;

   uint64_t k1 = 0, k2 = 0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
   switch ((sizeof c) % 16) {
   case 12: k2 ^= (uint64_t)tail[11] << 24;
            k2 ^= (uint64_t)tail[10] << 16;
            k2 ^= (uint64_t)tail[ 9] <<  8;
            k2 ^= (uint64_t)tail[ 8] <<  0;
            k2 *= c2; k2 = k2 << 33 | k2 >> 31; k2 *= c1; h2 ^= k2;
   case  8: k1 ^= (uint64_t)tail[ 7] << 56;
            k1 ^= (uint64_t)tail[ 6] << 48;
            k1 ^= (uint64_t)tail[ 5] << 40;
            k1 ^= (uint64_t)tail[ 4] << 32;
   case  4: k1 ^= (uint64_t)tail[ 3] << 24;
            k1 ^= (uint64_t)tail[ 2] << 16;
            k1 ^= (uint64_t)tail[ 1] <<  8;
            k1 ^= (uint64_t)tail[ 0] <<  0;
            k1 *= c1; k1 = k1 << 31 | k1 >> 33; k1 *= c2; h1 ^= k1;
   case  0: (void)tail; break;
   }
#pragma clang diagnostic pop

   h1 ^= sizeof c;
   h2 ^= sizeof c;

   h1 += h2;
   h2 += h1;

   static const uint64_t m1 = 0xff51afd7ed558ccdULL,
                         m2 = 0xc4ceb9fe1a85ec53ULL;

   h1 ^= h1 >> 33; h1 *= m1; h1 ^= h1 >> 33; h1 *= m2; h1 ^= h1 >> 33;
   h2 ^= h2 >> 33; h2 *= m1; h2 ^= h2 >> 33; h2 *= m2; h2 ^= h2 >> 33;

   h1 += h2;
   // h2 += h1;

   return h1;
#else
#error No hash function for a size_t of this size!
#endif
}
