// File created: 2012-02-01 19:18:33

// Not built by itself, only #included into load-string.98.c.

#define get_aabbs         MUSHSPACE_CAT(get_aabbs,UTF)
#define newline           MUSHSPACE_CAT(newline,UTF)
#define get_aabbs_binary  MUSHSPACE_CAT(get_aabbs_binary,UTF)
#define binary_load_arr   MUSHSPACE_CAT(binary_load_arr,UTF)
#define binary_load_blank MUSHSPACE_CAT(binary_load_blank,UTF)
#define load_arr          MUSHSPACE_CAT(load_arr,UTF)
#define load_blank        MUSHSPACE_CAT(load_blank,UTF)

// These operate under the assumption that s points or should point to an ASCII
// character. They work for all UTF and constant-width formats, because an
// ASCII character is always encoded as exactly one code unit and that code
// unit can never be a part of a different code point's encoding.
#ifndef ASCII_NEXT
#define ASCII_READ(s) (*(s))
#define ASCII_NEXT(s) (*(s)++)
#define ASCII_PREV(s) (*--(s))
#endif

static void get_aabbs(
   const void*, const void*, mushcoords tgt, bool binary, musharr_mushaabb*);

#if MUSHSPACE_DIM >= 2
static bool newline(
   bool*, mushcoords*, mushcoords, musharr_mushbounds, size_t*, size_t*,
   mushcoords, size_t*, uint8_t*);
#endif

static size_t get_aabbs_binary(const C*, const C*, mushcoords, mushbounds*);

static void binary_load_arr(musharr_mushcell, void*);
static void binary_load_blank(size_t, void*);
static void load_arr(musharr_mushcell, void*,
                     size_t, size_t, size_t, size_t, uint8_t*);
static void load_blank(size_t, void*);

int MUSHSPACE_CAT(mushspace_load_string,UTF)(
   mushspace* space, const C* str, size_t len,
   mushcoords* end, mushcoords target, bool binary)
{
   const C *str_end = str + len;

   const void *p = str;
   int ret = load_string_generic(
      space, &p, str_end, end, target, binary,
      get_aabbs, load_arr, load_blank, binary_load_arr, binary_load_blank);

   if (ret == MUSHERR_NONE) {
      str = p;
      assert (str <= str_end);
      while (str < str_end) {
         C c = ASCII_NEXT(str);
         assert (c == ' ' || c == '\r' || c == '\n' || c == '\f');
      }
      assert (str == str_end);
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
   const void* vstr, const void* vend, mushcoords target, bool binary,
   musharr_mushaabb* aabbs_out)
{
   static mushaabb aabbs[1 << MUSHSPACE_DIM];

   mushbounds *bounds = (mushbounds*)aabbs;
   const C *str = vstr, *str_end = vend;

   aabbs_out->ptr = aabbs;

   if (binary) {
      size_t n = get_aabbs_binary(str, str_end, target, bounds);
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

   mushcell c;
   while (str < str_end) {
      NEXT(str, str_end, c);
      switch (c) {
      default:
         #if MUSHSPACE_DIM >= 2
            if (got_cr)
               hit_newline;
         #endif

         if (c != ' ') {
            found_nonspace_for = found_nonspace_for_anyone = a;
            last_nonspace = pos;

            if (get_beg) for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
               if (get_beg & 1 << i) {
                  mushcell_min_into(&bounds[a].beg.v[i], pos.v[i]);
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
   const C* str, const C* str_end, mushcoords target, mushbounds* bounds)
{
   mushcoords beg = target;

   const C* str_trimmed_beg = str;
   for (;;) {
      if (str_trimmed_beg == str_end) {
         // All spaces: nothing to load.
         return 0;
      }
      if (ASCII_READ(str_trimmed_beg) != ' ')
         break;
      (void)ASCII_NEXT(str_trimmed_beg);
   }

   const size_t leading_spaces = str_trimmed_beg - str;

   beg.x += leading_spaces;

   // No need to check bounds here since we've already established that it's
   // not all spaces.
   const C* str_trimmed_end = str_end;
   while (ASCII_PREV(str_trimmed_end) == ' ');
   (void)ASCII_NEXT(str_trimmed_end);

   const size_t trailing_spaces = str_end - str_trimmed_end;

   // A good compiler should be able to optimize the loop away for non-UTF.
   size_t codepoints = 0;
   for (const C* p = str; p < str_trimmed_end; ++codepoints) {
      mushcell c;
      NEXT(p, str_trimmed_end, c);
      (void)c;
   }

   const size_t loadee_len         = codepoints - trailing_spaces,
                loadee_trimmed_len = loadee_len - leading_spaces;

   if (loadee_trimmed_len > (size_t)MUSHCELL_MAX) {
      // Oops, that's not going to fit! Bail.
      return SIZE_MAX;
   }

   mushcoords end = target;
   size_t a = 0;

   if (target.x > MUSHCELL_MAX - (mushcell)loadee_len) {
      end.x = MUSHCELL_MAX;
      bounds[a++] = (mushbounds){beg, end};
      beg.x = end.x = MUSHCELL_MIN;
   }
   end.x += loadee_len - 1;

   bounds[a++] = (mushbounds){beg, end};
   return a;
}

static void binary_load_arr(musharr_mushcell arr, void* p) {
   binary_load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;
   for (mushcell *end = arr.ptr + arr.len; arr.ptr < end; ++arr.ptr) {
      assert (str < str_end);
      mushcell c;
      NEXT(str, str_end, c);
      if (c != ' ')
         *arr.ptr = c;
   }
   aux->str = str;
}

static void binary_load_blank(size_t blanks, void* p) {
   binary_load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;
   (void)str_end;
   while (blanks) {
      assert (str < str_end);
      if (ASCII_READ(str) != ' ')
         break;
      --blanks;
      (void)ASCII_NEXT(str);
   }
   aux->str = str;
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
   const C *str = aux->str, *str_end = aux->end;

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

      mushcell c;
      NEXT(str, str_end, c);
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
   #endif
   #if MUSHSPACE_DIM < 3
      case '\f':
   #endif
         break;
      }

   #if MUSHSPACE_DIM >= 2
      case '\r':
         if (str < str_end && ASCII_READ(str) == '\n')
            (void)ASCII_NEXT(str);
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
         C c;
         while ((c = ASCII_READ(str)) == '\r' || c == '\n' || c == ' ')
            (void)ASCII_NEXT(str);

         if (str < str_end) {
            assert (c == '\f');
            (void)ASCII_NEXT(str);
         }
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      if (*hit & 1 << 0) {
         C c;
         while ((c = ASCII_READ(str)) == ' '
             || (MUSHSPACE_DIM < 3 && c == '\f'))
         {
            (void)ASCII_NEXT(str);
         }

         if (str < str_end) {
            assert (c == '\r' || c == '\n');
            if (ASCII_NEXT(str) == '\r' && str < str_end
             && ASCII_READ(str) == '\n')
            {
               (void)ASCII_NEXT(str);
            }
         }
         goto end;
      }
   #endif

   // See if we've just barely hit an EOL/EOP and report it if so. This is just
   // an optimization, we'd catch it and handle it appropriately come next call
   // anyway.
   #if MUSHSPACE_DIM >= 3
      if (ASCII_READ(str) == '\f') {
         (void)ASCII_NEXT(str);
         *hit = 1 << 1;
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      if (ASCII_READ(str) == '\r') {
         (void)ASCII_NEXT(str);
         *hit = 1 << 0;
      }
      if (str < str_end && ASCII_READ(str) == '\n') {
         (void)ASCII_NEXT(str);
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
   const C *str = aux->str, *str_end = aux->end;
   (void)str_end;
   while (blanks) {
      assert (str < str_end);
      C c = ASCII_READ(str);
      if (!(c == ' ' || c == '\r' || c == '\n' || c == '\f'))
         break;
      --blanks;
      (void)ASCII_NEXT(str);
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
#undef C
#undef NEXT
