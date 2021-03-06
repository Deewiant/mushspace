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

static void get_aabbs(const void*, const void*, mushcoords tgt, bool binary,
                      musharr_mushbounds*, mushspace*);

static size_t get_aabbs_binary(const C*, const C*, mushcoords, mushbounds*,
                               mushspace*);

static void binary_load_arr(musharr_mushcell, mushcoords, mushcoords, void*);
static void binary_load_blank(mushcoords, mushcoords, void*);
static void load_arr(musharr_mushcell, mushcoords, mushcoords, void*,
                     const mushbounds*, size_t, size_t, size_t, size_t,
                     uint8_t*);
static void load_blank(mushcoords, mushcoords, void*);

void MUSHSPACE_CAT(mushspace_load_string,UTF)(
   mushspace* space, const C* str, size_t len,
   mushcoords* end, mushcoords target, bool binary)
{
   const C *str_end = str + len;

   const void *p = str;
   load_string_generic(
      space, &p, str_end, end, target, binary,
      get_aabbs, load_arr, load_blank, binary_load_arr, binary_load_blank);

   str = p;
   assert (str <= str_end);
#ifdef MUSH_ENABLE_EXPENSIVE_DEBUGGING
   while (str < str_end) {
      C c = ASCII_NEXT(str);
      assert (c == ' ' || c == '\r' || c == '\n' || c == '\f');
   }
   assert (str == str_end);
#elif !defined(NDEBUG)
   // Just check one character: O(1) instead of O(n).
   if (str < str_end) {
      C c = ASCII_NEXT(str);
      assert (c == ' ' || c == '\r' || c == '\n' || c == '\f');
   }
#endif
}

// Writes AABBs to bounds_out->ptr describing where the input should be loaded,
// and sets bounds_out->len to the appropriate count. bounds_out->ptr should
// have room for 2^dim bounds; in binary mode, at most 2.
//
// Uses the given space only for signaling errors.
static void get_aabbs(
   const void* vstr, const void* vend, mushcoords target, bool binary,
   musharr_mushbounds* bounds_out, mushspace* space)
{
   const C *str = vstr, *str_end = vend;

   mushbounds *bounds = bounds_out->ptr;

   static const size_t BOUNDS_LEN = 1 << MUSHSPACE_DIM;

   if (binary) {
      const size_t n = get_aabbs_binary(str, str_end, target, bounds, space);
      assert (n <= 2);
      bounds_out->len = n;
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
   size_t found_nonspace_for = BOUNDS_LEN;

   // Not per-box: if this remains unchanged, we don't need to load a thing.
   size_t found_nonspace_for_anyone = BOUNDS_LEN;

   // A bit of a HACK to make sure that we don't mess up the starting
   // Y-coordinate.
   //
   // TODO: there should be a better solution than this.
   #if MUSHSPACE_DIM >= 3
      size_t found_nonspace_on_page = found_nonspace_for;
   #endif

   for (size_t i = 0; i < BOUNDS_LEN; ++i) {
      bounds[i].beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
      bounds[i].end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
   }

   #if MUSHSPACE_DIM >= 2
      bool got_cr = false;
   #endif

   // A helper we'll be using a number of times.
   #define UPDATE_BOUNDS_WITH_LAST_NONSPACE do { \
      if (found_nonspace_for == a) \
         mushcoords_max_into(&bounds[a].end, last_nonspace); \
   } while (0)

   while (str < str_end) {
      mushcell c;
      NEXT(str, str_end, c);

      #if MUSHSPACE_DIM >= 2
         if (got_cr || c == '\n') {
            got_cr = false;

            pos.x = target.x;

            if ((pos.y = mushcell_inc(pos.y)) == MUSHCELL_MIN) {
               UPDATE_BOUNDS_WITH_LAST_NONSPACE;

               found_nonspace_for = BOUNDS_LEN;

               #if MUSHSPACE_DIM >= 3
                  found_nonspace_on_page = found_nonspace_for;
               #endif

               max_a = mush_size_t_max(max_a, a |= 0x02);

            } else if (pos.y == target.y)
               mushspace_signal(space, MUSHERR_NO_ROOM, space);
            else if (a & 0x01)
               UPDATE_BOUNDS_WITH_LAST_NONSPACE;
            else if (found_nonspace_for == a)
               mushcell_max_into(&bounds[a].end.x, last_nonspace.x);

            a &= ~0x01;

            // If we've found a nonspace in these bounds, we don't need to look
            // for a smaller Y-coordinate: our Y will never get smaller since
            // we just saw this line break.
            if (found_nonspace_for == a) {
#if MUSHSPACE_DIM >= 3
               // But only if we've seen a nonspace on this page. Otherwise we
               // might still run into a smaller Y, in which case we leave
               // get_beg as-is.
               if (found_nonspace_on_page == a)
#endif
                  get_beg = 0x01;
            } else
               get_beg = DimensionBits;
         }
      #endif

      switch (c) {
      default:
         found_nonspace_for = found_nonspace_for_anyone = a;
         #if MUSHSPACE_DIM >= 3
            found_nonspace_on_page = found_nonspace_for;
         #endif
         last_nonspace = pos;

         if (get_beg) {
            for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
               if (get_beg & 1 << i)
                  mushcell_min_into(&bounds[a].beg.v[i], pos.v[i]);
            get_beg = 0;
         }
      case ' ':
         if ((pos.x = mushcell_inc(pos.x)) == MUSHCELL_MIN) {
            UPDATE_BOUNDS_WITH_LAST_NONSPACE;

            found_nonspace_for = BOUNDS_LEN;
            #if MUSHSPACE_DIM >= 3
               found_nonspace_on_page = found_nonspace_for;
            #endif
            get_beg = DimensionBits;

            max_a = mush_size_t_max(max_a, a |= 0x01);

         } else if (pos.x == target.x) {
            // Oops, came back to where we started. That's not good.
            mushspace_signal(space, MUSHERR_NO_ROOM, space);
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

            pos.x = target.x;
            pos.y = target.y;

            if ((pos.z = mushcell_inc(pos.z)) == MUSHCELL_MIN) {
               UPDATE_BOUNDS_WITH_LAST_NONSPACE;

               found_nonspace_for = BOUNDS_LEN;

               max_a = mush_size_t_max(max_a, a |= 0x04);

            } else if (pos.z == target.z)
               mushspace_signal(space, MUSHERR_NO_ROOM, space);
            else if (a & 0x03)
               UPDATE_BOUNDS_WITH_LAST_NONSPACE;
            else if (found_nonspace_for == a) {
               mushcell_max_into(&bounds[a].end.x, last_nonspace.x);
               mushcell_max_into(&bounds[a].end.y, last_nonspace.y);
            }

            a &= ~0x03;
            get_beg = found_nonspace_for == a ? 0x03 : DimensionBits;
            found_nonspace_on_page = BOUNDS_LEN;
         #endif
         break;
      }
   }

   if (found_nonspace_for_anyone == BOUNDS_LEN) {
      // Nothing to load.
      bounds_out->len = 0;
      return;
   }

   UPDATE_BOUNDS_WITH_LAST_NONSPACE;

   #undef UPDATE_BOUNDS_WITH_LAST_NONSPACE

   // Since a is a bitmask, the bounds that we used aren't necessarily in
   // order. Also, they might not all be initialized. Fix that.

   size_t n = 0;
   for (size_t i = 0; i <= max_a; ++i) {
      const mushbounds *box = &bounds[i];

      if (!(box->beg.x == MUSHCELL_MAX && box->end.x == MUSHCELL_MIN)) {
         // The box has been initialized, so make sure it's valid and put it in
         // place.

         for (mushdim j = 0; j < MUSHSPACE_DIM; ++j)
            assert (box->beg.v[j] <= box->end.v[j]);

         if (n != i)
            bounds[n] = bounds[i];
         ++n;
      }
   }
   bounds_out->len = n;
}

static size_t get_aabbs_binary(
   const C* str, const C* str_end, mushcoords beg, mushbounds* bounds,
   mushspace* space)
{
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

   const size_t wrap_after_idx = (size_t)MUSHCELL_MAX - (size_t)beg.x;

   size_t  prewrap_spaces = 0,
          postwrap_spaces = 0,
          codepoints;

#ifdef IS_UTF
   codepoints = 0;
   bool wrap_spaces_done = false;
   for (const C* p = str_trimmed_beg; p < str_trimmed_end; ++codepoints) {
      mushcell c;
      NEXT(p, str_trimmed_end, c);

      if (wrap_spaces_done)
         continue;

      if (codepoints <= wrap_after_idx)
         prewrap_spaces = c == ' ' ? prewrap_spaces+1 : 0;
      else if (c == ' ')
         ++postwrap_spaces;
      else
         wrap_spaces_done = true;
   }

   const bool wrapped = beg.x > MUSHCELL_MAX - (mushcell)(codepoints - 1);
#else
   // Help the poor compiler out.
   codepoints = str_trimmed_end - str_trimmed_beg;

   const bool wrapped = beg.x > MUSHCELL_MAX - (mushcell)(codepoints - 1);

   if (wrapped) {
      for (const C* p = str_trimmed_beg + wrap_after_idx; *++p == ' ';)
         ++postwrap_spaces;
      for (const C* p = str_trimmed_beg + wrap_after_idx; *p-- == ' ';)
         ++prewrap_spaces;
   }
#endif

   if (codepoints >
          mush_size_t_add_clamped(
             (size_t)MUSHCELL_MAX - (size_t)MUSHCELL_MIN, 1))
   {
      // Oops, that's not going to fit! Bail.
      mushspace_signal(space, MUSHERR_NO_ROOM, space);
   }

   mushcoords end = beg;
   size_t a = 0;

   if (wrapped) {
      end.x = MUSHCELL_MAX - prewrap_spaces;
      bounds[a++] = (mushbounds){beg, end};

      end.x = mushcell_sub(MUSHCELL_MIN, end.x - beg.x + 1);
      beg.x = MUSHCELL_MIN + postwrap_spaces;

      codepoints -= prewrap_spaces;
   }
   end.x += codepoints - 1;

   bounds[a++] = (mushbounds){beg, end};
   return a;
}

static void binary_load_arr(
   musharr_mushcell arr, mushcoords cbeg, mushcoords cend, void* p)
{
   (void)cbeg; (void)cend;

   binary_load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;

   while (ASCII_READ(str) == ' ')
      (void)ASCII_NEXT(str);

   for (mushcell *end = arr.ptr + arr.len; arr.ptr < end; ++arr.ptr) {
      assert (str < str_end);
      mushcell c;
      NEXT(str, str_end, c);
      if (c != ' ')
         *arr.ptr = c;
   }
   aux->str = str;
}

static void binary_load_blank(mushcoords beg, mushcoords end, void* p) {
   binary_load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;

   size_t blanks = (size_t)mushcell_sub(end.x, beg.x);

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
   musharr_mushcell arr, mushcoords arr_beg, mushcoords arr_end, void* p,
   const mushbounds* box_bounds,
   size_t width, size_t area, size_t line_start, size_t page_start,
   uint8_t* hit)
{
   #if MUSHSPACE_DIM == 1
   (void)box_bounds; (void)width; (void)line_start;
   (void)area; (void)page_start; (void)hit;
   #elif MUSHSPACE_DIM == 2
   (void)area; (void)page_start;
   #endif
   (void)arr_end;

   load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;

   // These bounds can be unsafe, as they encompass the whole loaded area and
   // thus may have wrapped along any number of axes.
   const mushbounds *bounds = aux->bounds;

   mushcoords pos = aux->pos;

   #if MUSHSPACE_DIM >= 2
      const mushcell target_x = aux->target_x;
   #if MUSHSPACE_DIM >= 3
      const mushcell target_y = aux->target_y;
   #endif
   #endif

   // If we're not in bounds, eat whitespace without affecting the array index
   // until we hit the array. At that point, the array index does start to
   // matter. (We should hit the array correctly at all other times.)
   //
   // Two examples. Consider the target position as (1000,1000,1000) in each.
   //
   // 1. [3d, bounds beg (1000,1000,1001)] FF LF FF data...
   //
   // The FF brings us to (1000,1000,1001) without affecting the array index.
   // Then the LF and FF should affect it as usual.
   //
   // 2. [3d, bounds beg (1001,1000,1000)] LF space data...
   //
   // The LF brings us to (1000,1001,1000). The next space then brings us to
   // (1001,1001,1000).
   //
   // Here, the line feed should have affected the array index, even though
   // it's not in bounds. It's not clear how this should be figured out in
   // advance, if that's even possible.
   //
   // But as soon as we jump into bounds we can compute the correct index based
   // on how pos differs compared to bounds->beg. This is what we do here.
   //
   // This can only happen when this is the first call to this function, i.e.
   // when we're loading into the first box. We shouldn't touch i otherwise.
   //
   // HACK HACK HACK: evidently it does happen at other times as well, so
   // instead we only modify i when this is the first call, and never
   // otherwise. Should figure out either a proper justification or a better
   // solution.
   size_t i = 0;
   if (!mushbounds_safe_contains(bounds, pos)) {
      assert (mushbounds_safe_contains(bounds, arr_beg));

      // Use load_blank, as it's smart enough to not eat whitespace that
      // shouldn't be eaten yet.
      load_blank(arr_beg, arr_beg, aux);
      if (!mushcoords_equal(aux->pos, arr_beg))
         return;

      pos = aux->pos;
      str = aux->str;

      if (aux->first) {
      #if MUSHSPACE_DIM >= 2
         const size_t old_line_start = line_start;
      #endif
      #if MUSHSPACE_DIM >= 3
         line_start = page_start += (size_t)(pos.z - bounds->beg.z) * area;
      #endif
      #if MUSHSPACE_DIM >= 2
         line_start              += (size_t)(pos.y - bounds->beg.y) * width;

         // Only set i to line_start if there were any lines. Otherwise
         // line_start may refer to an out-of-bounds location.
         if (line_start != old_line_start)
            i = line_start;
      #endif
         i                       += (size_t)(pos.x - bounds->beg.x);
      }
   }
   aux->first = false;

   while (i < arr.len && str < str_end) {
      mushcell c;
      NEXT(str, str_end, c);
      switch (c) {

      default:
         arr.ptr[i++] = c;
         pos.x = mushcell_inc(pos.x);
         break;

      case ' ':
         // Disregard trailing spaces.
         //
         // The fundamental reason for doing this is that our loop terminates
         // once i reaches arr.len. Spaces can cause us to overrun that
         // temporarily, before a line/page break snaps us back to a
         // still-in-range line_start/page_start.
         //
         // Consider this 2d case, where the s's represent spaces:
         //
         // +--+
         // |xx|ssss
         // |xx|
         // +--+
         //
         // After the second space, we've reached arr.len, since we increment i
         // for each one and arr.len is 4 due to the 2x2 box. So we need to
         // handle them specially, such as like this.
         //
         // Nonspaces can also cause us to fall out of bounds->end, which is
         // why we check for being past of it instead of on it, like in the
         // line break case below.
         #if MUSHSPACE_DIM >= 2
            if (pos.x == mushcell_inc(bounds->end.x))
               break;
         #endif
         ++i;
         pos.x = mushcell_inc(pos.x);

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
         #if MUSHSPACE_DIM >= 3
            if (pos.y == bounds->end.y)
               break;
         #endif

         i = line_start += width;
         pos.x = target_x;
         pos.y = mushcell_inc(pos.y);

         if (i >= arr.len || !mushbounds_contains(box_bounds, pos)) {
            // The next cell we would want to load falls on the next line,
            // which is in another box: report that.
            *hit = 1 << 0;
            aux->str = str;
            aux->pos = pos;
            return;
         }

         // There may be leading spaces on the next line. Eat them.
         while (pos.x != bounds->beg.x && str < str_end) {
            // The aabb in aabb_beg is specifically the one containing str, so
            // if our line_start is outside it that must mean that there are
            // spaces on the beginning of every line.

            const mushcell c2 = ASCII_READ(str);
            (void)ASCII_NEXT(str);
            switch (c2) {
            case ' ': pos.x = mushcell_inc(pos.x); break;

            // If a line containing only whitespace doesn't reach aabb_beg,
            // that's fine.
            case '\n': goto CASE_LF;
            case '\r': goto CASE_CR;

            #if MUSHSPACE_DIM >= 3
            case '\f': goto CASE_FF;
            #else
            case '\f': break;
            #endif

            default: MUSH_UNREACHABLE("nonspace out of bounds on new line");
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

         if (i >= arr.len || !mushbounds_contains(box_bounds, pos)) {
            *hit = 1 << 1;
            aux->str = str;
            aux->pos = pos;
            return;
         }

         // There may be leading spaces and/or line breaks on the next page.
         // Eat them.
         while ((pos.x != bounds->beg.x || pos.y != bounds->beg.y)
             && str < str_end)
         {
            const mushcell c2 = ASCII_READ(str);
            (void)ASCII_NEXT(str);
            switch (c2) {
            case ' ':
               pos.x = mushcell_inc(pos.x);
               break;

            case '\r':
               if (str < str_end && ASCII_READ(str) == '\n')
                  (void)ASCII_NEXT(str);
            case '\n':
               // We're already at the correct Y, so we move away from it. This
               // is like any other line break, and their case can handle the
               // leading spaces remaining.
               if (pos.y == bounds->beg.y)
                  goto CASE_LF;

               pos.x = target_x;
               pos.y = mushcell_inc(pos.y);
               break;

            // If a page containing only whitespace doesn't reach aabb_beg,
            // that's fine.
            case '\f': goto CASE_FF;

            default: MUSH_UNREACHABLE("nonspace out of bounds on new page");
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
         C c = 0;
         while (str < str_end
             && ((c = ASCII_NEXT(str)) == '\r' || c == '\n' || c == ' '));

         if (str < str_end) {
            assert (c == '\f');
            pos.x = target_x;
            pos.y = target_y;
            pos.z = mushcell_inc(pos.z);
         }
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      if (*hit & 1 << 0) {
         C c;
         while (str < str_end
             && ((c = ASCII_NEXT(str)) == ' '
         #if MUSHSPACE_DIM < 3
              || c == '\f'
         #endif
         ));

         if (str < str_end) switch (c) {
            case '\r':
               if (str < str_end && ASCII_READ(str) == '\n')
                  (void)ASCII_NEXT(str);
            case '\n':
               pos.x = target_x;
               pos.y = mushcell_inc(pos.y);
               break;
         #if MUSHSPACE_DIM >= 3
            case '\f':
               *hit = 1 << 1;
               pos.x = target_x;
               pos.y = target_y;
               pos.z = mushcell_inc(pos.z);
               break;
         #endif
            default: MUSH_UNREACHABLE("expected line break in string");
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
         pos.x = target_x;
         pos.y = target_y;
         pos.z = mushcell_inc(pos.z);
         goto end;
      }
   #endif
   #if MUSHSPACE_DIM >= 2
      bool newline = false;
      if (ASCII_READ(str) == '\r') {
         (void)ASCII_NEXT(str);
         if (str < str_end && ASCII_READ(str) == '\n')
            (void)ASCII_NEXT(str);
         newline = true;
      } else if (ASCII_READ(str) == '\n') {
         (void)ASCII_NEXT(str);
         newline = true;
      }
      if (newline) {
         *hit = 1 << 0;
         pos.x = target_x;
         pos.y = mushcell_inc(pos.y);
      }
   #endif
   #if MUSHSPACE_DIM >= 2
end:
   #endif
   aux->str = str;
   aux->pos = pos;
}

static void load_blank(mushcoords beg, mushcoords end, void* p) {
   (void)beg;

   load_arr_auxdata *aux = p;
   const C *str = aux->str, *str_end = aux->end;

   if (str == str_end)
      return;

   mushcoords pos = aux->pos;

   #if MUSHSPACE_DIM >= 2
      const mushcell target_x = aux->target_x;
   #if MUSHSPACE_DIM >= 3
      const mushcell target_y = aux->target_y;
   #endif
   #endif

   // The purpose of this function is only to bring str in line with where
   // loading is happening in space. It's entirely possible that we have 5
   // blank cells in space that we're asked to skip over in one call, but we
   // don't care because the next thing in the string is a newline. We do
   // nothing until the space also realizes that it's supposed to be on the
   // next line, at which point we agree to skip the newline and everything
   // works out.
   for (; str < str_end && !mushcoords_equal(pos, end);
          (void)ASCII_NEXT(str))
   {
      switch (ASCII_READ(str)) {
      case ' ':
         // The reason to abort here would be if pos == end, but that would've
         // ended the loop already, so never abort here.
         pos.x = mushcell_inc(pos.x);
         continue;

   #if MUSHSPACE_DIM >= 2
      #define Y_ONWARDS ((MUSHSPACE_DIM-1) * sizeof(mushcell))
      case '\r':
         if (!memcmp(pos.v + 1, end.v + 1, Y_ONWARDS))
            break;
         if (str+1 < str_end && ASCII_READ(str+1) == '\n')
            (void)ASCII_NEXT(str);
         if (false) {
      case '\n':
            if (!memcmp(pos.v + 1, end.v + 1, Y_ONWARDS))
               break;
         }
         pos.x = target_x;
         pos.y = mushcell_inc(pos.y);
         continue;
      #undef Y_ONWARDS
   #else
      case '\r': case '\n': continue;
   #endif
      case '\f':
         #if MUSHSPACE_DIM >= 3
            if (pos.z == end.z)
               break;
            pos.x = target_x;
            pos.y = target_y;
            pos.z = mushcell_inc(pos.z);
         #endif
         continue;

      default: break;
      }
      break;
   }
   aux->str = str;
   aux->pos = pos;
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
