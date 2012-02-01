// File created: 2012-02-01 19:18:33

// Not built by itself, only #included into load-string.98.c.

#define get_aabbs         MUSHSPACE_CAT(get_aabbs,UTF)
#define newline           MUSHSPACE_CAT(newline,UTF)
#define get_aabbs_binary  MUSHSPACE_CAT(get_aabbs_binary,UTF)
#define binary_load_arr   MUSHSPACE_CAT(binary_load_arr,UTF)
#define binary_load_blank MUSHSPACE_CAT(binary_load_blank,UTF)
#define load_arr          MUSHSPACE_CAT(load_arr,UTF)
#define load_blank        MUSHSPACE_CAT(load_blank,UTF)

static void get_aabbs(
   const void*, size_t, mushcoords target, bool binary, musharr_mushaabb*);

#if MUSHSPACE_DIM >= 2
static bool newline(
   bool*, mushcoords*, mushcoords, musharr_mushbounds, size_t*, size_t*,
   mushcoords, size_t*, uint8_t*);
#endif

static size_t get_aabbs_binary(
   const unsigned char*, size_t len, mushcoords target, mushbounds*);

static void binary_load_arr(musharr_mushcell, void*);
static void binary_load_blank(size_t, void*);
static void load_arr(musharr_mushcell, void*,
                     size_t, size_t, size_t, size_t, uint8_t*);
static void load_blank(size_t, void*);

int MUSHSPACE_CAT(mushspace_load_string,UTF)(
   mushspace* space, const unsigned char* str, size_t len,
   mushcoords* end, mushcoords target, bool binary)
{
   const unsigned char *str_end = str + len;

   const void *p = str;
   int ret = load_string_generic(
      space, &p, len, end, target, binary,
      get_aabbs, load_arr, load_blank, binary_load_arr, binary_load_blank);

   if (ret == MUSHERR_NONE) {
      str = p;
      for (; str < str_end; ++str)
         assert (*str == ' ' || *str == '\r' || *str == '\n' || *str == '\f');
      assert (!(str > str_end));
   }
   return ret;
}

// Sets aabbs_out to an array of AABBs (a slice out of a static buffer) which
// describe where the input should be loaded. There are at most 2^dim of them;
// in binary mode, at most 2.
//
// If nothing would be loaded, aabbs_out->ptr is set to NULL and an error code
// (an int) is written into aabbs_out->len.
static void get_aabbs(
   const void* vstr, size_t len, mushcoords target, bool binary,
   musharr_mushaabb* aabbs_out)
{
   static mushaabb aabbs[1 << MUSHSPACE_DIM];

   mushbounds *bounds = (mushbounds*)aabbs;
   const unsigned char *str = vstr;

   aabbs_out->ptr = aabbs;

   if (binary) {
      size_t n = get_aabbs_binary(str, len, target, bounds);
      if (n == SIZE_MAX) {
         *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NO_ROOM};
         return;
      }
      assert (n <= 2);
      aabbs_out->len = n;
      return;
   }

   // The index a as used below is a bitmask of along which axes pos
   // overflowed. Thus it changes over time as we read something like:
   //
   //          |
   //   foobarb|az
   //      qwer|ty
   // ---------+--------
   //      arst|mei
   //     qwfp |
   //          |
   //
   // After the ending 'p', a will not have its maximum value, which was in the
   // "mei" quadrant. So we have to keep track of it separately.
   size_t a = 0, max_a = 0;

   mushcoords pos = target;

   // All bits set up to the MUSHSPACE_DIM'th.
   static const uint8_t DimensionBits = (1 << MUSHSPACE_DIM) - 1;

   // A bitmask of which axes we want to search for the beginning point for.
   // Reset completely at overflows and partially at line and page breaks.
   uint8_t get_beg = DimensionBits;

   // We want minimal boxes, and thus exclude spaces at edges. These are
   // helpers toward that. lastNonSpace points to the last found nonspace
   // and foundNonSpaceFor is the index of the box it belonged to.
   mushcoords last_nonspace = target;
   size_t found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

   // Not per-box: if this remains unchanged, we don't need to load a thing.
   size_t found_nonspace_for_anyone = MUSH_ARRAY_LEN(aabbs);

   for (size_t i = 0; i < MUSH_ARRAY_LEN(aabbs); ++i) {
      bounds[i].beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
      bounds[i].end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
   }

   #if MUSHSPACE_DIM >= 2
      bool got_cr = false;

      const musharr_mushbounds bounds_arr = {bounds, MUSH_ARRAY_LEN(aabbs)};

      #define hit_newline do { \
         if (!newline(&got_cr, &pos, target, \
                      bounds_arr, &a, &max_a, \
                      last_nonspace, &found_nonspace_for, &get_beg))\
         { \
            *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NO_ROOM}; \
            return; \
         } \
      } while (0)
   #endif

   for (const unsigned char* str_end = str + len; str < str_end; ++str)
   switch (*str) {
   default:
      #if MUSHSPACE_DIM >= 2
         if (got_cr)
            hit_newline;
      #endif

      if (*str != ' ') {
         found_nonspace_for = found_nonspace_for_anyone = a;
         last_nonspace = pos;

         if (get_beg) for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
            if (get_beg & 1 << i) {
               bounds[a].beg.v[i] = mushcell_min(bounds[a].beg.v[i], pos.v[i]);
               get_beg &= ~(1 << i);
            }
         }
      }
      if ((pos.x = mushcell_inc(pos.x)) == MUSHCELL_MIN) {
         if (found_nonspace_for == a)
            mushcoords_max_into(&bounds[a].end, last_nonspace);

         found_nonspace_for = MUSH_ARRAY_LEN(aabbs);
         get_beg = DimensionBits;

         max_a = mush_size_t_max(max_a, a |= 0x01);

      } else if (pos.x == target.x) {
         // Oops, came back to where we started. That's not good.
         *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NO_ROOM};
         return;
      }
      break;

   case '\r':
      #if MUSHSPACE_DIM >= 2
         got_cr = true;
      #endif
      break;

   case '\n':
      #if MUSHSPACE_DIM >= 2
         hit_newline;
      #endif
      break;

   case '\f':
      #if MUSHSPACE_DIM >= 2
         if (got_cr)
            hit_newline;
      #endif
      #if MUSHSPACE_DIM >= 3
         mushcell_max_into(&bounds[a].end.x, last_nonspace.x);
         mushcell_max_into(&bounds[a].end.y, last_nonspace.y);

         pos.x = target.x;
         pos.y = target.y;

         if ((pos.z = mushcell_inc(pos.z)) == MUSHCELL_MIN) {
            if (found_nonspace_for == a)
               mushcoords_max_into(&bounds[a].end, last_nonspace);

            found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

            max_a = mush_size_t_max(max_a, a |= 0x04);

         } else if (pos.z == target.z) {
            *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NO_ROOM};
            return;
         }
         a &= ~0x03;
         get_beg = found_nonspace_for == a ? 0x03 : 0x07;
      #endif
      break;
   }
   #if MUSHSPACE_DIM >= 2
   #undef hit_newline
   #endif

   if (found_nonspace_for_anyone == MUSH_ARRAY_LEN(aabbs)) {
      // Nothing to load. Not an error, but don't need to do anything so bail.
      *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NONE};
      return;
   }

   if (found_nonspace_for < MUSH_ARRAY_LEN(aabbs))
      mushcoords_max_into(&bounds[found_nonspace_for].end, last_nonspace);

   // Since a is a bitmask, the AABBs that we used aren't necessarily in order.
   // Fix that.
   size_t n = 0;
   for (size_t i = 0; i <= max_a; ++i) {
      const mushbounds *box = &bounds[i];

      if (!(box->beg.x == MUSHCELL_MAX && box->end.x == MUSHCELL_MIN)) {
         // The box has been initialized, so make sure it's valid and put it in
         // place.

         for (mushdim j = 0; j < MUSHSPACE_DIM; ++j)
            assert (box->beg.v[j] <= box->end.v[j]);

         if (i != n)
            bounds[n] = bounds[i];
         ++n;
      }
   }
   aabbs_out->len = n;
}

#if MUSHSPACE_DIM >= 2
static bool newline(
   bool* got_cr, mushcoords* pos, mushcoords target,
   musharr_mushbounds bounds, size_t* a, size_t* max_a,
   mushcoords last_nonspace, size_t* found_nonspace_for, uint8_t* get_beg)
{
   *got_cr = false;

   mushcell_max_into(&bounds.ptr[*a].end.x, last_nonspace.x);

   pos->x = target.x;

   if ((pos->y = mushcell_inc(pos->y)) == MUSHCELL_MIN) {
      if (*found_nonspace_for == *a)
         mushcoords_max_into(&bounds.ptr[*a].end, last_nonspace);

      *found_nonspace_for = bounds.len;

      *max_a = mush_size_t_max(*max_a, *a |= 0x02);
   } else if (pos->y == target.y)
      return false;

   *a &= ~0x01;
   *get_beg = *found_nonspace_for == *a ? 0x01 : 0x03;
   return true;
}
#endif

static size_t get_aabbs_binary(
   const unsigned char* str, size_t len,
   mushcoords target, mushbounds* bounds)
{
   size_t a = 0;
   mushcoords beg = target, end = target;

   size_t i = 0;
   while (i < len && str[i++] == ' ');

   if (i == len) {
      // All spaces: nothing to load.
      return 0;
   }

   beg.x += i-1;

   // No need to check bounds here since we've already established that it's
   // not all spaces.
   i = len;
   while (str[--i] == ' ');

   if (i > (size_t)MUSHCELL_MAX) {
      // Oops, that's not going to fit! Bail.
      return SIZE_MAX;
   }

   if (end.x > MUSHCELL_MAX - (mushcell)i) {
      end.x = MUSHCELL_MAX;
      bounds[a++] = (mushbounds){beg, end};
      beg.x = MUSHCELL_MIN;
   }
   end.x += i;

   bounds[a++] = (mushbounds){beg, end};
   return a;
}

static void binary_load_arr(musharr_mushcell arr, void* p) {
   const unsigned char **strp = p, *str = *strp;
   for (mushcell *end = arr.ptr + arr.len; arr.ptr < end; ++arr.ptr) {
      unsigned char c = *str++;
      if (c != ' ')
         *arr.ptr = c;
   }
   *strp = str;
}

static void binary_load_blank(size_t blanks, void* p) {
   const unsigned char **strp = p, *str = *strp;
   while (blanks) {
      if (*str != ' ')
         break;
      --blanks;
      ++str;
   }
   *strp = str;
}

static void load_arr(
   musharr_mushcell arr, void* p,
   size_t width, size_t area, size_t line_start, size_t page_start,
   uint8_t* hit)
{
   #if MUSHSPACE_DIM == 1
   (void)width; (void)area; (void)page_start; (void)hit;
   #elif MUSHSPACE_DIM == 2
   (void)area;
   #endif

   load_arr_auxdata *aux = p;
   const unsigned char *str = aux->str, *str_end = str + aux->len;

   // x and y are used only for skipping leading spaces/newlines and thus
   // aren't really representative of the cursor position at any point.
   #if MUSHSPACE_DIM >= 2
      mushcell x = aux->x;
      const mushcell target_x = aux->target_x, aabb_beg_x = aux->aabb_beg_x;
   #if MUSHSPACE_DIM >= 3
      mushcell y = aux->y;
      const mushcell target_y = aux->target_y, aabb_beg_y = aux->aabb_beg_y;
   #endif
   #endif

   for (size_t i = 0; i < arr.len;) {
      assert (str < str_end);

      const unsigned char c = *str++;
      switch (c) {
      default:
         arr.ptr[i++] = c;
         break;

      case ' ': {
         // Ignore leading spaces (west of aabb.beg.x)
         bool leading_space = i == line_start;
         #if MUSHSPACE_DIM >= 2
            leading_space &= (x = mushcell_inc(x)) < aabb_beg_x;
         #endif
         if (!leading_space)
            ++i;

   #if MUSHSPACE_DIM < 2
      case '\r': case '\n':
   #if MUSHSPACE_DIM < 3
      case '\f':
   #endif
   #endif
         break;
      }

   #if MUSHSPACE_DIM >= 2
      case '\r':
         if (str < str_end && *str == '\n')
            ++str;
      case '\n': {
         // Ignore leading newlines (north of aabb.beg.y)
         bool leading_newline = i == page_start;
         #if MUSHSPACE_DIM >= 3
            leading_newline &= (y = mushcell_inc(y)) < aabb_beg_y;
         #endif
         if (!leading_newline) {
            i = line_start += width;
            x = target_x;

            if (i >= arr.len) {
               // The next cell we would want to load falls on the next line:
               // report that.
               *hit = 1 << 0;

               // We set x above, no need to write it back.
               aux->str = str;
               #if MUSHSPACE_DIM >= 3
                  aux->y = y;
               #endif
               return;
            }
         }
         break;
      }
   #endif
   #if MUSHSPACE_DIM >= 3
      case '\f':
         // Ignore leading form feeds (above aabb.beg.z)
         if (i) {
            i = line_start = page_start += area;
            y = target_y;

            if (i >= arr.len) {
               *hit = 1 << 1;

               // We set y above, no need to write it back.
               aux->str = str;
               aux->x = x;
               return;
            }
         }
         break;
   #endif
      }
   }
   if (str >= str_end) {
      // x and y are needed no longer: no need to write them back.
      aux->str = str;
      return;
   }

   // When we've hit an EOL/EOP in Funge-Space, we need to skip any
   // trailing whitespace until we hit it in the file as well.
   #if MUSHSPACE_DIM == 3
      if (*hit & 1 << 1) {
         while (*str == '\r' || *str == '\n' || *str == ' ')
            ++str;

         if (str < str_end) {
            assert (*str == '\f');
            ++str;
         }
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      if (*hit & 1 << 0) {
         while (*str == ' ' || (MUSHSPACE_DIM < 3 && *str == '\f'))
            ++str;

         if (str < str_end) {
            assert (*str == '\r' || *str == '\n');
            if (*str++ == '\r' && str < str_end && *str == '\n')
               ++str;
         }
         goto end;
      }
   #endif

   // See if we've just barely hit an EOL/EOP and report it if so. This is just
   // an optimization, we'd catch it and handle it appropriately come next call
   // anyway.
   #if MUSHSPACE_DIM >= 3
      if (*str == '\f') {
         ++str;
         *hit = 1 << 1;
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      if (*str == '\r') {
         ++str;
         *hit = 1 << 0;
      }
      if (str < str_end && *str == '\n') {
         ++str;
         *hit = 1 << 0;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
end:
      aux->x = x;
   #if MUSHSPACE_DIM >= 3
      aux->y = y;
   #endif
   #endif
   aux->str = str;
   return;
}

static void load_blank(size_t blanks, void* p) {
   load_arr_auxdata *aux = p;
   const unsigned char *str = aux->str;
   while (blanks) {
      if (!(*str == ' ' || *str == '\r' || *str == '\n' || *str == '\f'))
         break;
      --blanks;
      ++str;
   }
   aux->str = str;
}

#undef get_aabbs
#undef newline
#undef get_aabbs_binary
#undef binary_load_arr
#undef binary_load_blank
#undef load_arr
#undef load_blank

#undef UTF