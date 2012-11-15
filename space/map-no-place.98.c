// File created: 2012-01-27 20:58:47

#include "space/map-no-place.98.h"

#include <alloca.h>
#include <assert.h>
#include <string.h>

typedef struct {
   mushcell cell;
   mushboxen_iter iter;
   bool static_box;
} mushcell_loc;

static bool map_in_box(
   mushspace*, mushbounded_pos, mushboxen_iter,
   void*, void(*f)(musharr_mushcell, mushcoords, mushcoords, void*));

static bool map_in_static(
   mushspace*, mushbounded_pos,
   void*, void(*f)(musharr_mushcell, mushcoords, mushcoords, void*));

static bool mapex_in_box(
   mushspace*, mushbounded_pos, mushboxen_iter, void*,
   void(*)(musharr_mushcell, mushcoords, mushcoords, void*,
           const mushbounds*, size_t, size_t, size_t, size_t, uint8_t*));

static bool mapex_in_static(
   mushspace*, mushbounded_pos, void*,
   void(*)(musharr_mushcell, mushcoords, mushcoords, void*,
           const mushbounds*, size_t, size_t, size_t, size_t, uint8_t*));

static bool get_next_in(
   const mushspace*, const mushbounds*, mushcoords*,
   void*, void(*g)(mushcoords, mushcoords, void*));

static void get_next_in1(
   mushdim, const mushbounds*, mushcoords, const mushbounds*, mushboxen_iter,
   mushcell_loc*, mushcell_loc*, mushcell_loc*);

static mushcoords get_end_of_contiguous_range(
   const mushbounds*, mushcoords*, const mushbounds*, bool*);

void mushspace_map_no_place(
   mushspace* space, const mushbounds* bounds, void* fg,
   void(*f)(musharr_mushcell, mushcoords, mushcoords, void*),
   void(*g)(mushcoords, mushcoords, void*))
{
   mushcoords       pos = bounds->beg;
   mushbounded_pos bpos = {bounds, &pos};

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (;;) next_pos: {
      assert (mushbounds_safe_contains(bounds, pos));

      if (mushstaticaabb_contains(pos)) {
         if (map_in_static(space, bpos, fg, f))
            return;

         if (!mushbounds_safe_contains(bounds, pos))
            goto get_next;

         goto next_pos;
      }

      mushboxen_iter iter = mushboxen_get_iter(&space->boxen, pos, aux);
      if (!mushboxen_iter_is_null(iter)) {
         if (map_in_box(space, bpos, iter, fg, f))
            return;

         if (!mushbounds_safe_contains(bounds, pos))
            goto get_next;

         goto next_pos;
      }

#if USE_BAKAABB
      if (space->bak.data && mushbounds_contains(&space->bak.bounds, pos)) {
         mushcell *p = mushbakaabb_get_ptr_unsafe(&space->bak, pos);
         f((musharr_mushcell){p,1}, pos, pos, fg);

         for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
            if (pos.v[i] != bounds->end.v[i]) {
               ++pos.v[i];
               goto next_pos;
            }
            pos.v[i] = bounds->beg.v[i];
         }
         return;
      }
#endif

      // No hits for pos: find the next pos we can hit, or stop if there's
      // nothing left.
get_next:
      if (!get_next_in(space, bounds, &pos, fg, g))
         return;
   }
}

static bool map_in_box(
   mushspace* space, mushbounded_pos bpos, mushboxen_iter iter,
   void* fdata, void(*f)(musharr_mushcell, mushcoords, mushcoords, void*))
{
   // Consider:
   //     +-----+
   // x---|░░░░░|---+
   // |░░░|░░░░░|░░░|
   // |░B░|░░A░░|░░░y
   // |   |     |   |
   // +---|     |---+
   //     +-----+
   // We want to map the range from x to y (shaded). Unless we tessellate,
   // we'll get the whole thing from box B straight away.

   const mushaabb *box = mushboxen_iter_box(iter);

   mushbounds tes = box->bounds;
   mushbounds_tessellate(&tes, *bpos.pos, &MUSHSTATICAABB_BOUNDS);

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_above it =
           mushboxen_iter_above_init(&space->boxen, iter, aux);
        !mushboxen_iter_above_done( it, &space->boxen);
         mushboxen_iter_above_next(&it, &space->boxen))
   {
      mushbounds_tessellate(
         &tes, *bpos.pos, &mushboxen_iter_above_box(it)->bounds);
   }

   bool hit_end;
   const mushcoords
      beg = *bpos.pos,
      end = get_end_of_contiguous_range(bpos.bounds, bpos.pos, &tes, &hit_end);

   const size_t
      beg_idx = mushaabb_get_idx(box, beg),
      end_idx = mushaabb_get_idx(box, end);

   assert (beg_idx <= end_idx);

   const size_t length = end_idx - beg_idx + 1;

   f((musharr_mushcell){box->data + beg_idx, length}, beg, end, fdata);
   return hit_end;
}

static bool map_in_static(
   mushspace* space, mushbounded_pos bpos,
   void* fdata, void(*f)(musharr_mushcell, mushcoords, mushcoords, void*))
{
   bool hit_end;
   const mushcoords
      beg = *bpos.pos,
      end = get_end_of_contiguous_range(
         bpos.bounds, bpos.pos, &MUSHSTATICAABB_BOUNDS, &hit_end);
   const size_t
      beg_idx = mushstaticaabb_get_idx(beg),
      end_idx = mushstaticaabb_get_idx(end);

   assert (beg_idx <= end_idx);

   const size_t length = end_idx - beg_idx + 1;

   f((musharr_mushcell){space->static_box.array + beg_idx, length}, beg, end,
     fdata);
   return hit_end;
}

// Passes some extra data to the function, for matching array index
// calculations with the location of the mushcell* (probably quite specific to
// file loading, where this is used):
//
// - The bounds of the enclosing box.
//
// - The width and area of the enclosing box.
//
// - The indices in the mushcell* of the previous line and page (note: always
//   zero or negative (thus big numbers, since unsigned)).
//
// - Whether a new line or page was just reached, with one bit for each boolean
//   (LSB for line, next-most for page). This may be updated by the function to
//   reflect that it's done with the line/page.
//
// Does not use bakaabb, and indeed cannot due to the above data not making
// sense with it.
void mushspace_mapex_no_place(
   mushspace* space, const mushbounds* bounds, void* fg,
   void(*f)(musharr_mushcell, mushcoords, mushcoords, void*,
            const mushbounds*, size_t, size_t, size_t, size_t, uint8_t*),
   void(*g)(mushcoords, mushcoords, void*))
{
   mushcoords       pos = bounds->beg;
   mushbounded_pos bpos = {bounds, &pos};

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (;;) next_pos: {
      assert (mushbounds_safe_contains(bounds, pos));

      if (mushstaticaabb_contains(pos)) {
         if (mapex_in_static(space, bpos, fg, f))
            return;

         if (!mushbounds_safe_contains(bounds, pos))
            goto get_next;

         goto next_pos;
      }

      mushboxen_iter iter = mushboxen_get_iter(&space->boxen, pos, aux);
      if (!mushboxen_iter_is_null(iter)) {
         if (mapex_in_box(space, bpos, iter, fg, f))
            return;

         if (!mushbounds_safe_contains(bounds, pos))
            goto get_next;

         goto next_pos;
      }

      // No hits for pos: find the next pos we can hit, or stop if there's
      // nothing left.
get_next:
      if (!get_next_in(space, bounds, &pos, fg, g))
         return;
   }
}

static bool mapex_in_box(
   mushspace* space, mushbounded_pos bpos, mushboxen_iter iter,
   void* caller_data,
   void(*f)(musharr_mushcell, mushcoords, mushcoords, void*,
            const mushbounds*, size_t, size_t, size_t, size_t, uint8_t*))
{
   size_t width, area, line_start, page_start;

   const mushaabb   *box    = mushboxen_iter_box(iter);
   const mushbounds *bounds = bpos.bounds;
   mushcoords       *pos    = bpos.pos;

   // These depend on the original pos and thus have to be initialized before
   // the call to get_end_of_contiguous_range.

#if MUSHSPACE_DIM >= 2
   // {box->bounds.beg.x, pos->y, pos->z}
   mushcoords ls = *pos;
   ls.x = box->bounds.beg.x;

   // {box->bounds.beg.x, box->bounds.beg.y, pos->z}
   mushcoords ps = box->bounds.beg;
   memcpy(ps.v + 2, pos->v + 2, (MUSHSPACE_DIM - 2) * sizeof(mushcell));
#endif

#if MUSHSPACE_DIM >= 2
   const mushcell prev_y = pos->y;
#if MUSHSPACE_DIM >= 3
   const mushcell prev_z = pos->z;
#endif
#endif

   mushbounds tes = box->bounds;
   mushbounds_tessellate(&tes, *pos, &MUSHSTATICAABB_BOUNDS);

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_above it =
           mushboxen_iter_above_init(&space->boxen, iter, aux);
        !mushboxen_iter_above_done( it, &space->boxen);
         mushboxen_iter_above_next(&it, &space->boxen))
   {
      mushbounds_tessellate(&tes, *pos, &mushboxen_iter_above_box(it)->bounds);
   }

   bool hit_end;
   const mushcoords
      beg = *pos,
      end = get_end_of_contiguous_range(bounds, pos, &tes, &hit_end);

   const size_t beg_idx = mushaabb_get_idx(box, beg),
                end_idx = mushaabb_get_idx(box, end);

   assert (beg_idx <= end_idx);

   uint8_t hit;

#if MUSHSPACE_DIM >= 2
   hit = 0;

   width = box->width;
   hit |= (pos->x == bounds->beg.x && pos->y != ls.y) << 0;

   line_start = mushaabb_get_idx(box, ls) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
   area = box->area;
   hit |= (pos->y == bounds->beg.y && pos->z != ls.z) << 1;

   page_start = mushaabb_get_idx(box, ps) - beg_idx;
#endif

   const size_t length = end_idx - beg_idx + 1;

   // Depending on MUSHSPACE_DIM all of width, area, line_start, and page_start
   // can be used uninitialized here, but that's fine.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
   f((musharr_mushcell){box->data + beg_idx, length}, beg, end, caller_data,
     &box->bounds, width, area, line_start, page_start, &hit);
#pragma GCC diagnostic pop

#if MUSHSPACE_DIM >= 2
   if (hit == 0x01 && pos->y == prev_y) {
      // f hit an EOL and pos->y hasn't been bumped, so bump it.
      pos->x = bounds->beg.x;
      if (pos->y == bounds->end.y) {
         #if MUSHSPACE_DIM >= 3
            goto bump_z;
         #else
            hit_end = true;
         #endif
      } else
         pos->y = mushcell_inc(pos->y);
   }
#endif
#if MUSHSPACE_DIM >= 3
   if (hit == 0x02 && pos->z == prev_z) {
      // Ditto for EOP.
      pos->x = bounds->beg.x;
bump_z:
      pos->y = bounds->beg.y;
      if (pos->z == bounds->end.z)
         hit_end = true;
      else
         pos->z = mushcell_inc(pos->z);
   }
#endif
   return hit_end;
}

static bool mapex_in_static(
   mushspace* space, mushbounded_pos bpos,
   void* caller_data,
   void(*f)(musharr_mushcell, mushcoords, mushcoords, void*,
            const mushbounds*, size_t, size_t, size_t, size_t, uint8_t*))
{
   size_t width, area, line_start, page_start;

   const mushbounds *bounds = bpos.bounds;
   mushcoords       *pos    = bpos.pos;

#if MUSHSPACE_DIM >= 2
   mushcoords ls = *pos;
   ls.x = MUSHSTATICAABB_BEG.x;

   mushcoords ps = MUSHSTATICAABB_BEG;
   memcpy(ps.v + 2, pos->v + 2, (MUSHSPACE_DIM - 2) * sizeof(mushcell));
#endif

#if MUSHSPACE_DIM >= 2
   const mushcell prev_y = pos->y;
#if MUSHSPACE_DIM >= 3
   const mushcell prev_z = pos->z;
#endif
#endif

   bool hit_end;
   const mushcoords
      beg = *pos,
      end = get_end_of_contiguous_range(
               bounds, pos, &MUSHSTATICAABB_BOUNDS, &hit_end);

   const size_t beg_idx = mushstaticaabb_get_idx(beg),
                end_idx = mushstaticaabb_get_idx(end);

   assert (beg_idx <= end_idx);

   uint8_t hit;

#if MUSHSPACE_DIM >= 2
   hit = 0;

   width = MUSHSTATICAABB_SIZE.x;
   hit |= (pos->x == bounds->beg.x && pos->y != ls.y) << 0;

   line_start = mushstaticaabb_get_idx(ls) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
   area = MUSHSTATICAABB_SIZE.x * MUSHSTATICAABB_SIZE.y;
   hit |= (pos->y == bounds->beg.y && pos->z != ls.z) << 1;

   page_start = mushstaticaabb_get_idx(ps) - beg_idx;
#endif

   const size_t length = end_idx - beg_idx + 1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
   f((musharr_mushcell){space->static_box.array + beg_idx, length}, beg, end,
     caller_data, &MUSHSTATICAABB_BOUNDS, width, area,
     line_start, page_start, &hit);
#pragma GCC diagnostic pop

#if MUSHSPACE_DIM >= 2
   if (hit == 0x01 && pos->y == prev_y) {
      pos->x = bounds->beg.x;
      if (pos->y == bounds->end.y) {
         #if MUSHSPACE_DIM >= 3
            goto bump_z;
         #else
            hit_end = true;
         #endif
      } else
         pos->y = mushcell_inc(pos->y);
   }
#endif
#if MUSHSPACE_DIM >= 3
   if (hit == 0x02 && pos->z == prev_z) {
      pos->x = bounds->beg.x;
bump_z:
      pos->y = bounds->beg.y;
      if (pos->z == bounds->end.z)
         hit_end = true;
      else
         pos->z = mushcell_inc(pos->z);
   }
#endif
   return hit_end;
}

// The next (linewise) allocated point after *pos which is also within the
// given bounds. Calls g with the first two arguments being the current point
// and the next point respectively.
//
// Assumes that that next point, if it exists, was allocated within some box:
// doesn't look at bakaabb at all.
static bool get_next_in(
   const mushspace* space, const mushbounds* bounds,
   mushcoords* pos, void* gdata, void(*g)(mushcoords, mushcoords, void*))
{
   const mushcoords orig = *pos;

restart:
   if (mushbounds_safe_contains(bounds, *pos)) {
      assert (!mushstaticaabb_contains(*pos));
      assert (!mushboxen_get(&space->boxen, *pos));
   }

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {

      // Check every box until we find the best allocated solution.

      // Separate solutions for being able to increment the coordinate, jumping
      // to the next box without wrapping, and jumping to the next box with
      // wrapping.
      mushcell_loc
         increment        = {.iter = mushboxen_iter_null, .static_box = false},
         best_beg         = {.iter = mushboxen_iter_null, .static_box = false},
         best_wrapped_beg = {.iter = mushboxen_iter_null, .static_box = false};

      get_next_in1(i, bounds, *pos, &MUSHSTATICAABB_BOUNDS,
                   mushboxen_iter_null,
                   &increment, &best_beg, &best_wrapped_beg);

      for (mushboxen_iter it = mushboxen_iter_init(&space->boxen, aux);
           !mushboxen_iter_done( it, &space->boxen);
            mushboxen_iter_next(&it, &space->boxen))
      {
         const mushaabb *box = mushboxen_iter_box(it);
         get_next_in1(i, bounds, *pos, &box->bounds, it,
                      &increment, &best_beg, &best_wrapped_beg);
      }

      #define TEST_COORD(p, cell_loc) do { \
         /* Since we want to constrain pos to be in bounds, finding a solution
          * along a non-X-axis implies that the lower axes get "reset" to
          * bounds->beg. (Just like a line break brings the X position to the
          * page's left edge.) */ \
         memcpy((p)->v, bounds->beg.v, i * sizeof(mushcell)); \
         \
         (p)->v[i] = (cell_loc).cell; \
         \
         /* We may not end up in bounds, or in any box: check for both. */ \
         if (!mushbounds_safe_contains(bounds, *(p))) \
            break; \
         if (!((   !(cell_loc).static_box \
                && mushbounds_contains( \
                      &mushboxen_iter_box((cell_loc).iter)->bounds, *(p))) \
         \
            /* If we ended up in some other box, that's fine as well. */ \
            || mushstaticaabb_contains(*(p)) \
            || mushboxen_get(&space->boxen, *(p)))) \
         { \
            break; \
         } \
         *pos = *(p); \
         g(orig, *(p), gdata); \
         assert (mushbounds_safe_contains(bounds, *pos)); \
         return true; \
      } while (0)

      if (increment.static_box || !mushboxen_iter_is_null(increment.iter)) {
         if (i) {
            // This is not the X-axis. If an increment here fails, we might
            // still be able to hit something post-increment on a previous
            // axis. So simply use this as the solution candidate.
            best_beg = increment;
         } else {
            // This is the X-axis. If an increment here fails, we can simply
            // jump into the best box: clearly we'll never end up in this box,
            // or the increment would've succeeded. So try the increment but
            // then fall back to best_beg as usual.
            mushcoords p = *pos;
            TEST_COORD(&p, increment);
         }
      }

      if (!best_beg.static_box && mushboxen_iter_is_null(best_beg.iter)) {
         if (!best_wrapped_beg.static_box
          && mushboxen_iter_is_null(best_wrapped_beg.iter))
         {
            // No solution along this axis: try the next one.
            continue;
         }

         // Take the wrapping solution as it's the only one available.
         best_beg = best_wrapped_beg;
      }

      TEST_COORD(pos, best_beg);

      // If we didn't hit any box, go again with the new *pos. Akin to
      // continuing along on the next line/page.
      goto restart;
   }
   return false;
}

static void get_next_in1(
   mushdim x, const mushbounds* bounds, mushcoords pos,
   const mushbounds* box_bounds, mushboxen_iter it,
   mushcell_loc* increment,
   mushcell_loc* best_coord, mushcell_loc* best_wrapped)
{
   // If the box begins later than the best solution we've found, there's no
   // point in looking further into it.
   //
   // This seems somewhat nontrivial in the presence of wraparound but I've
   // convinced myself that it's true.
   if (box_bounds->beg.v[x] >= best_coord->cell
    && (best_coord->static_box || !mushboxen_iter_is_null(best_coord->iter)))
      return;

   // If this box doesn't overlap with the AABB we're interested in, skip it.
   if (!mushbounds_safe_overlaps(bounds, box_bounds))
      return;

   // If pos has crossed an axis within the bounds, prevent us from grabbing a
   // new pos on the other side of the axis we're wrapped around, or we'll just
   // keep looping around that axis.
   if (pos.v[x] < bounds->beg.v[x] && box_bounds->beg.v[x] > bounds->end.v[x])
      return;

   const bool beg_available = mushbounds_safe_contains1(bounds, pos, x);

   mushcell_loc *finder;

   // The ordinary best solution that skips to another box is the minimal
   // box.beg greater than pos.
   if (beg_available && box_bounds->beg.v[x] > pos.v[x]) {
      best_coord->cell = box_bounds->beg.v[x];
      finder = best_coord;
      goto found;
   }

   // Alternatively, we have the solution of simply moving forward by one pos:
   // this is okay if we're in the box and the result is in bounds.
   //
   // As far as we can tell here, we are: beg <= pos < end, so pos+1 is fine.
   if (box_bounds->end.v[x] > pos.v[x]) {
      ++pos.v[x];
      if (!mushbounds_safe_contains1(bounds, pos, x))
         return;
      increment->cell = pos.v[x];
      finder = increment;
      goto found;
   }

   // If the path from pos to bounds->end requires a wraparound, take the
   // global minimum box.beg as a last-resort option if nothing else is found,
   // so that we wrap around if there's no non-wrapping solution.
   if (beg_available
    && pos.v[x] > bounds->end.v[x]
    && (box_bounds->beg.v[x] < best_wrapped->cell
     || (!best_wrapped->static_box
      && mushboxen_iter_is_null(best_wrapped->iter))))
   {
      best_wrapped->cell = box_bounds->beg.v[x];
      finder = best_wrapped;
      goto found;
   }
   return;
found:
   if (mushboxen_iter_is_null(it)) {
      finder->iter       = mushboxen_iter_null;
      finder->static_box = true;
   } else {
      finder->iter       = it;
      finder->static_box = false;
   }
}

// Returns the end of the longest contiguous (linewise) range starting from
// "from", in "bounds". If necessary, updates "from" to point to the start of
// the next range after this one. When the returned value is equal to
// bounds->end, "reached_end" is set to true.
//
// "tes_bounds" are the bounds in which we are allowed to be contiguous, due to
// tessellation. They are guaranteed to be safe, unlike "bounds".
mushcoords get_end_of_contiguous_range(
   const mushbounds* bounds,
   mushcoords*       from,
   const mushbounds* tes_bounds,
   bool*             reached_end)
{
#if MUSHSPACE_DIM >= 2
   mushcell orig_from[MUSHSPACE_DIM-1];
   memcpy(orig_from, from->v + 1, sizeof orig_from);
#endif
   *reached_end = false;

   // The end point of this contiguous range, which we'll update as needed and
   // eventually return.
   mushcoords end = tes_bounds->end;

   // Check all axes except for the last.
   mushdim i = 0;
#if MUSHSPACE_DIM >= 2
   for (; i < MUSHSPACE_DIM-1; ++i) {
      if (tes_bounds->end.v[i] == bounds->end.v[i]) {
         // We can reach the end of "bounds" on this axis: we'll be going to
         // the next line/page, then.
         from->v[i] = bounds->beg.v[i];
         continue;
      }

      const size_t remaining_bytes = (MUSHSPACE_DIM-(i+1)) * sizeof(mushcell);

      // Did not reach the end point: the remaining axes won't change.
      memcpy(end.v + i+1, from->v + i+1, remaining_bytes);

      if (end.v[i] < bounds->end.v[i] || from->v[i] > bounds->end.v[i]) {
         // Cannot reach the bounds->end on this axis: either because our
         // tessellation limits us or because it's wrapped around with respect
         // to "from", so it's not possible to reach it without going to
         // another box.
         //
         // We can reach the end of our tessellated bounds, though. And the
         // next point will simply follow that.
         from->v[i] = mushcell_inc(end.v[i]);
      } else {
         // end.v[i] >= bounds->end.v[i] && from->v[i] <= bounds->end.v[i]

         // We can reach bounds->end on this axis.
         end.v[i] = bounds->end.v[i];

         // If we happen to be at bounds->end on the remaining axes as well,
         // then we've reached it completely.
         if (!memcmp(end.v + i+1, bounds->end.v + i+1, remaining_bytes))
            *reached_end = true;
         else {
            // We should go further on the next axis next time around, since
            // we're done along this one.
            from->v[i]   = bounds->beg.v[i];
            from->v[i+1] = mushcell_inc(from->v[i+1]);
         }
      }
      goto verify;
   }
#endif
   // All axes except the last were checked and found to be reachable. Check
   // the last one analogously and set reached_end if we hit bounds->end.
   if (end.v[i] == bounds->end.v[i])
      *reached_end = true;
   else {
      if (end.v[i] < bounds->end.v[i] || from->v[i] > bounds->end.v[i])
         from->v[i] = mushcell_inc(end.v[i]);
      else {
         end.v[i] = bounds->end.v[i];
         *reached_end = true;
      }
   }
#if MUSHSPACE_DIM >= 2
verify:
   for (mushdim j = 1; j < MUSHSPACE_DIM; ++j) {
      const size_t S = sizeof(mushcell);

      // If we were going to cross a line/page but we're actually in a box
      // tessellated in such a way that we can't (x and/or y don't match in the
      // tessellation and the encompassing bounds), wibble things so that we
      // just go to the end of the line/page.
      if (end.v[j] > orig_from[j-1]
       && memcmp(tes_bounds->beg.v, bounds->beg.v, j * S))
      {
         const size_t remaining_bytes = (MUSHSPACE_DIM - j) * S;

         memcpy(end.v   + j,   orig_from + j-1, remaining_bytes);
         memcpy(from->v + j+1, orig_from + j,   remaining_bytes - S);
         from->v[j] = mushcell_inc(orig_from[j-1]);

         *reached_end = false;
         break;
      }
   }
#endif
   return end;
}
