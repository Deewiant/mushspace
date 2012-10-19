// File created: 2012-02-01 19:18:33

// Not built by itself, only #included into load-string.98.c.

#define get_aabbs         MUSHSPACE_CAT(get_aabbs,UTF)
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

      // aabbs overlaps bounds so we don't need to copy the first one.
      for (size_t i = 1; i < n; ++i)
         aabbs[i].bounds = bounds[i];
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
   // helpers toward that. last_nonspace points to the last found nonspace and
   // found_nonspace_for is the index of the box it belonged to.
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
   #endif

   mushcell c;
   while (str < str_end) {
      NEXT(str, str_end, c);

      #if MUSHSPACE_DIM >= 2
         if (got_cr || c == '\n') {
            got_cr = false;

            mushcell_max_into(&bounds[a].end.x, last_nonspace.x);

            pos.x = target.x;

            if ((pos.y = mushcell_inc(pos.y)) == MUSHCELL_MIN) {
               if (found_nonspace_for == a)
                  mushcoords_max_into(&bounds[a].end, last_nonspace);

               found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

               max_a = mush_size_t_max(max_a, a |= 0x02);

            } else if (pos.y == target.y) {
               *aabbs_out = (musharr_mushaabb){NULL, MUSHERR_NO_ROOM};
               return;
            }
            a &= ~0x01;
            get_beg = found_nonspace_for == a ? 0x01 : DimensionBits;
         }
      #endif

      switch (c) {
      default:
         found_nonspace_for = found_nonspace_for_anyone = a;
         last_nonspace = pos;

         if (get_beg) {
            for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
               if (get_beg & 1 << i)
                  mushcell_min_into(&bounds[a].beg.v[i], pos.v[i]);
            get_beg = 0;
         }
      case ' ':
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
      case '\n':
         break;

      case '\f':
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
            get_beg = found_nonspace_for == a ? 0x03 : DimensionBits;
         #endif
         break;
      }
   }

   if (found_nonspace_for_anyone == MUSH_ARRAY_LEN(aabbs)) {
      // Nothing to load.
      aabbs_out->len = 0;
      return;
   }

   if (found_nonspace_for < MUSH_ARRAY_LEN(aabbs))
      mushcoords_max_into(&bounds[found_nonspace_for].end, last_nonspace);

   // Since a is a bitmask, the AABBs that we used aren't necessarily in order.
   // Fix that.
   size_t n = 1;
   for (size_t i = 1; i <= max_a; ++i) {
      const mushbounds *box = &bounds[i];

      if (!(box->beg.x == MUSHCELL_MAX && box->end.x == MUSHCELL_MIN)) {
         // The box has been initialized, so make sure it's valid and put it in
         // place.

         for (mushdim j = 0; j < MUSHSPACE_DIM; ++j)
            assert (box->beg.v[j] <= box->end.v[j]);

         aabbs[n++].bounds = bounds[i];
      }
   }
   aabbs_out->len = n;
}

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

   // A good compiler should be able to optimize the loop away for non-UTF.
   size_t codepoints = 0;
   for (const C* p = str_trimmed_beg; p < str_trimmed_end; ++codepoints) {
      mushcell c;
      NEXT(p, str_trimmed_end, c);
      (void)c;
   }

   if (codepoints > (size_t)MUSHCELL_MAX) {
      // Oops, that's not going to fit! Bail.
      return SIZE_MAX;
   }

   mushcoords end = beg;
   size_t a = 0;

   if (target.x > MUSHCELL_MAX - (mushcell)(codepoints - 1)) {
      end.x = MUSHCELL_MAX;
      bounds[a++] = (mushbounds){beg, end};
      beg.x = end.x = MUSHCELL_MIN;
   }
   end.x += codepoints - 1;

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
   (void)width; (void)area; (void)line_start; (void)page_start; (void)hit;
   #elif MUSHSPACE_DIM == 2
   (void)area; (void)page_start;
   #endif

   load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;

   const mushcoords aabb_beg = aux->aabb_beg;

   // Pos is used only for skipping leading spaces/newlines and thus isn't
   // really representative of the cursor position at most points.
   mushcoords pos = aux->pos;

   #if MUSHSPACE_DIM >= 2
      const mushcell target_x = aux->target_x;
   #if MUSHSPACE_DIM >= 3
      const mushcell target_y = aux->target_y;
   #endif
   #endif

   // Careful!
   //
   // We *NEED* to use != to compare coordinates to aabb_beg here, throughout.
   // We only keep track of coordinates that are "less than" aabb_beg or
   // exactly on it, but it could be the case that target > aabb_beg because of
   // wraparound. We don't need to treat that case in any special way because
   // we don't actually need to do less-than comparisons: we can just use !=.

   // Ignore whitespace until the box's start. Note that we know by this point
   // that there's nonspace in the string, so we don't need to worry about
   // running out of its bounds.
   while (!mushcoords_equal(aabb_beg, pos)) {
      assert (str < str_end);

      const mushcell c = ASCII_READ(str);
      (void)ASCII_NEXT(str);
      switch (c) {
      case ' ':
         pos.x = mushcell_inc(pos.x);

   #if MUSHSPACE_DIM < 2
      case '\r': case '\n':
   #endif
   #if MUSHSPACE_DIM < 3
      case '\f':
   #endif
         break;

   #if MUSHSPACE_DIM >= 2
      case '\r':
         if (ASCII_READ(str) == '\n')
            (void)ASCII_NEXT(str);
      case '\n':
         pos.x = target_x;
         pos.y = mushcell_inc(pos.y);
         break;
   #endif

   #if MUSHSPACE_DIM >= 3
      case '\f':
         pos.x = target_x;
         pos.y = target_y;
         pos.z = mushcell_inc(pos.z);
         break;
   #endif

      default: assert (false);
      }
   }

   for (size_t i = 0; i < arr.len && str < str_end;) {
      mushcell c;
      NEXT(str, str_end, c);
      switch (c) {
      default:
         arr.ptr[i++] = c;
         break;

      case ' ':
         ++i;

   #if MUSHSPACE_DIM < 2
      case '\r': case '\n':
   #endif
   #if MUSHSPACE_DIM < 3
      case '\f':
   #endif
         break;

   #if MUSHSPACE_DIM >= 2
      CASE_CR:
      case '\r':
         if (str < str_end && ASCII_READ(str) == '\n')
            (void)ASCII_NEXT(str);
      CASE_LF:
      case '\n': {
         i = line_start += width;
         pos.x = target_x;
         pos.y = mushcell_inc(pos.y);

         if (i >= arr.len) {
            // The next cell we would want to load falls on the next line:
            // report that.
            *hit = 1 << 0;

            aux->str = str;
            aux->pos = pos;
            return;
         }

         // There may be leading spaces on the next line. Eat them.
         while (pos.x != aabb_beg.x && str < str_end) {
            // The aabb in aabb_beg is specifically the one containing str, so
            // if our line_start is outside it that must mean that there are
            // spaces on the beginning of every line.

            const mushcell c = ASCII_READ(str);
            (void)ASCII_NEXT(str);
            switch (c) {
            case ' ': pos.x = mushcell_inc(pos.x); break;

            // If a line containing only whitespace doesn't reach aabb_beg,
            // that's fine.
            case '\n': goto CASE_LF;
            case '\r': goto CASE_CR;
            case '\f':
               #if MUSHSPACE_DIM < 3
                  break;
               #else
                  goto CASE_FF;
               #endif

            default: assert (false);
            }
         }
         break;
      }
   #endif
   #if MUSHSPACE_DIM >= 3
      CASE_FF:
      case '\f':
         i = line_start = page_start += area;
         pos.x = target_x;
         pos.y = target_y;
         pos.z = mushcell_inc(pos.z);

         if (i >= arr.len) {
            *hit = 1 << 1;

            aux->str = str;
            aux->pos = pos;
            return;
         }

         // There may be leading spaces and/or line breaks on the next page.
         // Eat them.
         while ((pos.x != aabb_beg.x || pos.y != aabb_beg.y) && str < str_end) {
            const mushcell c = ASCII_READ(str);
            (void)ASCII_NEXT(str);
            switch (c) {
            case ' ':
               pos.x = mushcell_inc(pos.x);
               break;

            case '\r':
               if (str < str_end && ASCII_READ(str) == '\n')
                  (void)ASCII_NEXT(str);
            case '\n':
               pos.x = target_x;
               pos.y = mushcell_inc(pos.y);
               break;

            // If a page containing only whitespace doesn't reach aabb_beg,
            // that's fine.
            case '\f': goto CASE_FF;

            default: assert (false);
            }
         }
         break;
   #endif
      }
   }
   if (str >= str_end) {
      // pos is needed no longer: no need to write it back.
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
   #endif
   aux->str = str;
   aux->pos = pos;
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
