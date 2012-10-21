// File created: 2012-01-27 20:58:47

#include "double-size-t.any.h"
#include "space/map-no-place.98.h"

#include <assert.h>
#include <string.h>

typedef struct { mushcell cell; size_t idx; } mushcell_idx;

static bool map_in_box(mushspace*, mushbounded_pos, mushcaabb_idx,
                       void*, void(*f)(musharr_mushcell, void*));

static bool map_in_static(mushspace*, mushbounded_pos,
                          void*, void(*f)(musharr_mushcell, void*));

static bool mapex_in_box(
   mushspace*, mushbounded_pos, mushcaabb_idx, void*,
   void(*)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*));

static bool mapex_in_static(
   mushspace*, mushbounded_pos, void*,
   void(*)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*));

static bool get_next_in(
   const mushspace*, const mushbounds*, mushcoords*,
   void*, void(*g)(size_t, size_t, void*));

static void get_next_in1(
   mushucell, const mushbounds*, mushcell, size_t, mushcoords, size_t,
   mushcell_idx*, mushcell_idx*);

void mushspace_map_no_place(
   mushspace* space, const mushbounds* bounds, void* fg,
   void(*f)(musharr_mushcell, void*), void(*g)(size_t, size_t, void*))
{
   mushcoords       pos = bounds->beg;
   mushbounded_pos bpos = {bounds, &pos};

   for (;;) next_pos: {
      if (mushstaticaabb_contains(pos)) {
         if (map_in_static(space, bpos, fg, f))
            return;
         else
            goto next_pos;
      }

      for (size_t b = 0; b < space->box_count; ++b) {
         mushcaabb_idx box = mushspace_get_caabb_idx(space, b);

         if (!mushbounds_contains(&box.aabb->bounds, pos))
            continue;

         if (map_in_box(space, bpos, box, fg, f))
            return;
         else
            goto next_pos;
      }

      if (mushbounds_contains(&space->bak.bounds, pos)) {
         mushcell *p = mushbakaabb_get_ptr_unsafe(&space->bak, pos);
         f((musharr_mushcell){p,1}, fg);

         for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
            if (pos.v[i] != bounds->end.v[i]) {
               ++pos.v[i];
               goto next_pos;
            }
            pos.v[i] = bounds->beg.v[i];
         }
         return;
      }

      // No hits for pos: find the next pos we can hit, or stop if there's
      // nothing left.
      if (!get_next_in(space, bounds, &pos, fg, g))
         return;
   }
}

static bool map_in_box(
   mushspace* space, mushbounded_pos bpos, mushcaabb_idx cai,
   void* caller_data, void(*f)(musharr_mushcell, void*))
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

   const mushaabb *box = cai.aabb;

   mushbounds tes = box->bounds;
   mushbounds_tessellate(&tes, *bpos.pos, &MUSHSTATICAABB_BOUNDS);
   for (size_t i = 0; i < cai.idx; ++i)
      mushbounds_tessellate(&tes, *bpos.pos, &space->boxen[i].bounds);

   bool hit_end;
   const size_t
      beg_idx = mushaabb_get_idx(box, *bpos.pos),
      end_idx = mushaabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
         tes.end, bpos.pos, bpos.bounds->end,
         bpos.bounds->beg, &hit_end, tes.beg, box->bounds.beg));

   assert (beg_idx <= end_idx);

   const size_t length = end_idx - beg_idx + 1;

   f((musharr_mushcell){box->data + beg_idx, length}, caller_data);
   return hit_end;
}

static bool map_in_static(
   mushspace* space, mushbounded_pos bpos,
   void* caller_data, void(*f)(musharr_mushcell, void*))
{
   bool hit_end;
   const size_t
      beg_idx = mushstaticaabb_get_idx(*bpos.pos),
      end_idx = mushstaticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
         MUSHSTATICAABB_END, bpos.pos, bpos.bounds->end, bpos.bounds->beg,
         &hit_end, MUSHSTATICAABB_BEG, MUSHSTATICAABB_BEG));

   assert (beg_idx <= end_idx);

   const size_t length = end_idx - beg_idx + 1;

   f((musharr_mushcell){space->static_box.array + beg_idx, length},
     caller_data);
   return hit_end;
}

// Passes some extra data to the function, for matching array index
// calculations with the location of the mushcell* (probably quite specific to
// file loading, where this is used):
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
   void(*f)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*),
   void(*g)(size_t, size_t, void*))
{
   mushcoords       pos = bounds->beg;
   mushbounded_pos bpos = {bounds, &pos};

   for (;;) next_pos: {
      if (mushstaticaabb_contains(pos)) {
         if (mapex_in_static(space, bpos, fg, f))
            return;
         else
            goto next_pos;
      }

      for (size_t b = 0; b < space->box_count; ++b) {
         mushcaabb_idx box = mushspace_get_caabb_idx(space, b);

         if (!mushbounds_contains(&box.aabb->bounds, pos))
            continue;

         if (mapex_in_box(space, bpos, box, fg, f))
            return;
         else
            goto next_pos;
      }

      // No hits for pos: find the next pos we can hit, or stop if there's
      // nothing left.
      if (!get_next_in(space, bounds, &pos, fg, g))
         return;
   }
}

static bool mapex_in_box(
   mushspace* space, mushbounded_pos bpos,
   mushcaabb_idx cai,
   void* caller_data,
   void(*f)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*))
{
   size_t width, area, line_start, page_start;

   const mushaabb   *box    = cai.aabb;
   const mushbounds *bounds = bpos.bounds;
   mushcoords       *pos    = bpos.pos;

   // These depend on the original pos and thus have to be initialized before
   // the call to mushcoords_get_end_of_contiguous_range.

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
   for (size_t i = 0; i < cai.idx; ++i)
      mushbounds_tessellate(&tes, *pos, &space->boxen[i].bounds);

   bool hit_end;
   const size_t
      beg_idx = mushaabb_get_idx(box, *pos),
      end_idx = mushaabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
         tes.end, pos, bounds->end,
         bounds->beg, &hit_end, tes.beg, box->bounds.beg));

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

   // Depending on MUSHSPACE_DIM all of width, area, and page_start can be used
   // uninitialized here, but that's fine.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
   f((musharr_mushcell){box->data + beg_idx, length},
     caller_data, width, area, line_start, page_start, &hit);
#pragma GCC diagnostic pop

#if MUSHSPACE_DIM >= 2
   if (hit == 0x01 && pos->y == prev_y) {
      // f hit an EOL and pos->y hasn't been bumped, so bump it.
      pos->x = bounds->beg.x;
      if ((pos->y = mushcell_inc(pos->y)) > bounds->end.y) {
         #if MUSHSPACE_DIM >= 3
            goto bump_z;
         #else
            hit_end = true;
         #endif
      }
   }
#endif
#if MUSHSPACE_DIM >= 3
   if (hit == 0x02 && pos->z == prev_z) {
      // Ditto for EOP.
      pos->x = bounds->beg.x;
bump_z:
      pos->y = bounds->beg.y;
      if ((pos->z = mushcell_inc(pos->z)) > bounds->end.z)
         hit_end = true;
   }
#endif
   return hit_end;
}

static bool mapex_in_static(
   mushspace* space, mushbounded_pos bpos,
   void* caller_data,
   void(*f)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*))
{
   size_t width, area, line_start, page_start;

   const mushbounds *bounds = bpos.bounds;
   mushcoords        *pos    = bpos.pos;

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
   size_t
      beg_idx = mushstaticaabb_get_idx(*pos),
      end_idx = mushstaticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
         MUSHSTATICAABB_END, pos, bounds->end,
         bounds->beg, &hit_end, MUSHSTATICAABB_BEG, MUSHSTATICAABB_BEG));

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
   f((musharr_mushcell){space->static_box.array + beg_idx, length},
     caller_data, width, area, line_start, page_start, &hit);
#pragma GCC diagnostic pop

#if MUSHSPACE_DIM >= 2
   if (hit == 0x01 && pos->y == prev_y) {
      pos->x = bounds->beg.x;
      if ((pos->y = mushcell_inc(pos->y)) > bounds->end.y) {
         #if MUSHSPACE_DIM >= 3
            goto bump_z;
         #else
            hit_end = true;
         #endif
      }
   }
#endif
#if MUSHSPACE_DIM >= 3
   if (hit == 0x02 && pos->z == prev_z) {
      pos->x = bounds->beg.x;
bump_z:
      pos->y = bounds->beg.y;
      if ((pos->z = mushcell_inc(pos->z)) > bounds->end.z)
         hit_end = true;
   }
#endif
   return hit_end;
}

// The next (linewise) allocated point after *pos which is also within the
// given bounds. Calls g with the first two arguments being the number of
// unallocated cells skipped (equivalently to a mush_double_size_t). This may
// require multiple calls.
//
// Assumes that that next point, if it exists, was allocated within some box:
// doesn't look at bakaabb at all.
static bool get_next_in(
   const mushspace* space, const mushbounds* bounds,
   mushcoords* pos, void* gdata, void(*g)(size_t, size_t, void*))
{
restart:
   {
      assert (!mushstaticaabb_contains(*pos));
      assert (!mushspace_find_box(space, *pos));
   }

   const size_t box_count = space->box_count;

   // A value of box_count here refers to the static box.
   //
   // Separate solutions for the best non-wrapping and the best wrapping
   // coordinate, with the wrapping coordinate used only if a non-wrapping
   // solution is not found.
   mushcell_idx
      best_coord   = {.idx = box_count + 1},
      best_wrapped = {.idx = box_count + 1};

   bool found = false;
   mush_double_size_t skipped = {0,0};

   // A helper to make sure that skipped doesn't overflow.
   #define SAFE_ADD(x) do { \
      if (skipped.hi > SIZE_MAX - (x).hi) { \
         g(x.hi, x.lo, gdata); \
         x = (mush_double_size_t){0,0}; \
      } \
      mush_double_size_t_add_into(&skipped, x); \
   } while (0)

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {

      // Go through every box, starting from the static one.

      get_next_in1(i, bounds, pos->v[i], box_count,
                   MUSHSTATICAABB_BEG, box_count,
                   &best_coord, &best_wrapped);

      for (mushucell b = 0; b < box_count; ++b)
         get_next_in1(i, bounds, pos->v[i], box_count,
                      space->boxen[b].bounds.beg, b,
                      &best_coord, &best_wrapped);

      if (best_coord.idx > box_count) {
         if (best_wrapped.idx > box_count) {
            // No solution: look along the next axis.
            continue;
         }

         // Take the wrapping solution as it's the only one available.
         best_coord = best_wrapped;
      }

      const mushcoords old = *pos;

      memcpy(pos->v, bounds->beg.v, i * sizeof(mushcell));
      pos->v[i] = best_coord.cell;

      // Old was already a space, or we wouldn't've called this function in the
      // first place. (See assertions.) Hence skipped is always at least one.
      mush_double_size_t_add_into(&skipped, (mush_double_size_t){0,1});

      mushdim j;
#if MUSHSPACE_DIM >= 2
      for (j = 0; j < MUSHSPACE_DIM-1; ++j) {
         mush_double_size_t volume = mushbounds_volume_on(bounds, j);
         size_t volume_skips = mushcell_sub(bounds->end.v[j], old.v[j]);

         // Make sure the multiplication doesn't overflow.
         if (volume.hi)
            while (volume_skips > SIZE_MAX / volume.hi)
               g(volume.hi, volume.lo, gdata);

         mush_double_size_t_mul1_into(&volume, volume_skips);
         SAFE_ADD(volume);
      }
#else
      // Avoid "condition is always true" warnings by doing this instead of the
      // above loop.
      j = MUSHSPACE_DIM - 1;
#endif

      mush_double_size_t volume = mushbounds_volume_on(bounds, j);
      size_t volume_skips = mushcell_dec(mushcell_sub(pos->v[j], old.v[j]));

      // All-zero means (SIZE_MAX+1) * (SIZE_MAX+1). This can only happen with
      // three or more dimensions, and with three dimensions only here (on the
      // last axis).
      if (MUSHSPACE_DIM == 3 && volume.hi == 0 && volume.lo == 0) {
         volume.lo = 1;
         for (size_t i = volume_skips; i--;)
            g(SIZE_MAX, SIZE_MAX, gdata);
      }

      if (volume.hi)
         while (volume_skips > SIZE_MAX / volume.hi)
            g(volume.hi, volume.lo, gdata);
      mush_double_size_t_mul1_into(&volume, volume_skips);
      SAFE_ADD(volume);

      // When memcpying pos->v above, we may not end up in any box.

      if (// If we didn't memcpy it's a guaranteed hit.
          !i

          // If we ended up in the box, that's fine too.
       || (   best_coord.idx < box_count
           && mushbounds_contains(&space->boxen[best_coord.idx].bounds, *pos))

          // If we ended up in some other box, that's also fine.
       || mushstaticaabb_contains(*pos) || mushspace_find_box(space, *pos))
      {
         found = true;
         break;
      }

      // Otherwise, go again with the new *pos.
      goto restart;
   }
   if (skipped.hi || skipped.lo)
      g(skipped.hi, skipped.lo, gdata);
   return found;
}

static void get_next_in1(
   mushucell x, const mushbounds* bounds, mushcell posx, size_t box_count,
   mushcoords box_beg, size_t box_idx,
   mushcell_idx* best_coord, mushcell_idx* best_wrapped)
{
   assert (best_wrapped->cell <= best_coord->cell);

   // If the box begins later than the best solution we've found, there's no
   // point in looking further into it.
   if (box_beg.v[x] >= best_coord->cell && best_coord->idx == box_count+1)
      return;

   // If this box doesn't overlap with the AABB we're interested in, skip it.
   if (!mushbounds_safe_contains(bounds, box_beg))
      return;

   // If pos has crossed an axis within the AABB, prevent us from grabbing a
   // new pos on the other side of the axis we're wrapped around, or we'll just
   // keep looping around that axis.
   if (posx < bounds->beg.v[x] && box_beg.v[x] > bounds->end.v[x])
      return;

   // If the path from pos to bounds->end requires a wraparound, take the
   // global minimum box.beg as a last-resort option if nothing else is found,
   // so that we wrap around if there's no non-wrapping solution.
   //
   // Note that best_wrapped->cell <= best_coord->cell so we can safely test
   // this after the first best_coord->cell check.
   if (posx > bounds->end.v[x]
    && (box_beg.v[x] < best_wrapped->cell || best_wrapped->idx == box_count+1))
   {
      best_wrapped->cell = box_beg.v[x];
      best_wrapped->idx  = box_idx;

   // The ordinary best solution is the minimal box.beg greater than pos.
   } else if (box_beg.v[x] > posx) {
      best_coord->cell = box_beg.v[x];
      best_coord->idx  = box_idx;
   }
}
