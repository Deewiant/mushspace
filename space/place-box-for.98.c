// File created: 2012-01-28 00:01:32

#include "space/place-box-for.98.h"

#include <alloca.h>
#include <assert.h>

#include "space/place-box.98.h"

static void get_box_for(mushspace*, mushcoords, mushaabb*);

static bool get_box_along_recent_line_for(mushspace*, mushcoords, mushaabb*);

static bool get_box_along_recent_volume_for(
   const mushspace*, mushcoords, mushaabb*);

static bool extend_big_sequence_start_for(
   const mushspace*, mushcoords, const mushbounds*, mushaabb*);

static bool extend_first_placed_big_for(
   const mushspace*, mushcoords, const mushbounds*, mushaabb*);

int mushspace_place_box_for(
   mushspace* space, mushcoords c, mushaabb** placed)
{
#if USE_BAKAABB
   if (mushboxen_count(&space->boxen) >= MAX_PLACED_BOXEN)
      return MUSHERR_OOM;
#endif

   mushaabb aabb;
   get_box_for(space, c, &aabb);

   int err = mushspace_place_box(space, &aabb, &c, placed);
   if (err && err != MUSHERR_INVALIDATION_FAILURE)
      return err;

   if (!*placed)
      *placed = mushboxen_get(&space->boxen, c);

   assert (mushbounds_contains(&(*placed)->bounds, c));

   mushmemorybuf_push(
      &space->recent_buf, (mushmemory){.placed = (*placed)->bounds, c});

   return err;
}

static void get_box_for(mushspace* space, mushcoords c, mushaabb* aabb) {
#ifdef MUSH_ENABLE_EXPENSIVE_DEBUGGING
   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter it = mushboxen_iter_init(&space->boxen, aux);
        !mushboxen_iter_done( it, &space->boxen);
         mushboxen_iter_next(&it, &space->boxen))
   {
      assert (!mushbounds_contains(&mushboxen_iter_box(it)->bounds, c));
   }
#endif

   if (space->recent_buf.full) {
      if (space->just_placed_big) {
         if (get_box_along_recent_volume_for(space, c, aabb))
            goto end;
      } else
         if (get_box_along_recent_line_for(space, c, aabb))
            goto end;
   }

   space->just_placed_big = false;

   mushbounds bounds = {mushcoords_subs_clamped(c, NEWBOX_PAD),
                        mushcoords_adds_clamped(c, NEWBOX_PAD)};
   mushaabb_make(aabb, &bounds);

end:
   assert (mushbounds_safe_contains(&aabb->bounds, c));
}

static bool get_box_along_recent_line_for(
   mushspace* space, mushcoords c, mushaabb* aabb)
{
   // Detect mushspace_put patterns where we seem to be moving in a straight
   // line and allocate one long, narrow box. This is the first big box
   // allocation so it has to look for a sequence of small boxes in
   // space->recent_buf:
   //
   // AAABBBCCC
   // AAABBBCCCc
   // AAABBBCCC
   //
   // Where A, B, and C are distinct boxes and c is c.

   assert (space->recent_buf.full);

   mushmemory recents[MUSHMEMORYBUF_SIZE];
   mushmemorybuf_read(&space->recent_buf, recents);

   // Find the axis along which the first two recent placements are aligned, if
   // any, and note whether it was along the positive or negative direction.
   bool positive;
   mushdim axis = MUSHSPACE_DIM;

   for (mushdim d = 0; d < MUSHSPACE_DIM; ++d) {
      mushcell diff = mushcell_sub(recents[1].c.v[d], recents[0].c.v[d]);

      if (axis == MUSHSPACE_DIM) {
         if (diff > NEWBOX_PAD && diff <= NEWBOX_PAD + BIG_SEQ_MAX_SPACING) {
            positive = true;
            axis = d;
            continue;
         }
         if (diff < -NEWBOX_PAD && diff >= -NEWBOX_PAD - BIG_SEQ_MAX_SPACING) {
            positive = false;
            axis = d;
            continue;
         }
      }
      if (diff)
         return false;
   }
   if (axis == MUSHSPACE_DIM)
      return false;

   // Check that c itself is on the same line.
   {
      for (mushdim d = 0; d < axis; ++d)
         if (c.v[d] != recents[MUSH_ARRAY_LEN(recents)-1].c.v[d])
            return false;
      for (mushdim d = axis+1; d < MUSHSPACE_DIM; ++d)
         if (c.v[d] != recents[MUSH_ARRAY_LEN(recents)-1].c.v[d])
            return false;

      mushcell diff =
         mushcell_sub(c.v[axis], recents[MUSH_ARRAY_LEN(recents)-1].c.v[axis]);
      if (!positive)
         diff *= -1;

      if (diff <= NEWBOX_PAD || diff > NEWBOX_PAD + BIG_SEQ_MAX_SPACING)
         return false;
   }

   // Check that the other recents are aligned on the same line.
   for (size_t i = 1; i < MUSH_ARRAY_LEN(recents) - 1; ++i) {

      const mushcell *a = recents[i].c.v, *b = recents[i+1].c.v;

      for (mushdim d = 0; d < axis; ++d)
         if (a[d] != b[d])
            return false;
      for (mushdim d = axis+1; d < MUSHSPACE_DIM; ++d)
         if (a[d] != b[d])
            return false;

      mushcell diff = mushcell_sub(b[axis], a[axis]);
      if (!positive)
         diff *= -1;

      if (diff <= NEWBOX_PAD || diff > NEWBOX_PAD + BIG_SEQ_MAX_SPACING)
         return false;
   }

   space->just_placed_big    = true;
   space->first_placed_big   = c;
   space->big_sequence_start = recents[0].c;

   mushbounds bounds = {c,c};
   if (positive) mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
   else          mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);

   mushaabb_make_unsafe(aabb, &bounds);
   return true;
}

static bool get_box_along_recent_volume_for(
   const mushspace* space, mushcoords c, mushaabb* aabb)
{
   assert (space->recent_buf.full);
   assert (space->just_placed_big);

   const mushbounds *last = &mushmemorybuf_last(&space->recent_buf)->placed;

   if (extend_big_sequence_start_for(space, c, last, aabb))
      return true;

   if (extend_first_placed_big_for(space, c, last, aabb))
      return true;

   return false;
}

static bool extend_big_sequence_start_for(
   const mushspace* space, mushcoords c, const mushbounds* last,
   mushaabb* aabb)
{
   // See if c is at big_sequence_start except for one axis, along which it's
   // just past last->end or last->beg. The typical case for this is the
   // following:
   //
   // sBBBBBBc
   //
   // Where B are boxes, s is space->big_sequence_start, and c is c.

   bool positive;
   mushdim axis = MUSHSPACE_DIM;

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
      if (axis == MUSHSPACE_DIM) {
         if (c.v[i] >  last->end.v[i]
          && c.v[i] <= last->end.v[i] + BIG_SEQ_MAX_SPACING)
         {
            positive = true;
            axis = i;
            continue;
         }
         if (c.v[i] <  last->beg.v[i]
          && c.v[i] >= last->beg.v[i] - BIG_SEQ_MAX_SPACING)
         {
            positive = false;
            axis = i;
            continue;
         }
      }
      if (c.v[i] != space->big_sequence_start.v[i])
         return false;
   }
   if (axis == MUSHSPACE_DIM)
      return false;

   // Extend last along the axis where c was outside it.
   mushbounds bounds = *last;
   if (positive) mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
   else          mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);

   mushaabb_make_unsafe(aabb, &bounds);
   return true;
}

static bool extend_first_placed_big_for(
   const mushspace* space, mushcoords c, const mushbounds* last,
   mushaabb* aabb)
{
   // Match against space->first_placed_big. This is for the case when we've
   // made a few non-big boxes and then hit a new dimension for the first time
   // in a location which doesn't match with the actual box. E.g.:
   //
   // BsBfBBB
   // BBBc  b
   //  n
   //
   // B being boxes, s being space->big_sequence_start, f being
   // space->first_placed_big, and c being c. b and n are explained below.
   //
   // So what we want is that c is "near" (up to BIG_SEQ_MAX_SPACING away from)
   // space->first_placed_big on one axis. In the diagram, this corresponds to
   // c being one line below f.
   //
   // We also want it to match space->first_placed_big on the other axes. This
   // prevents us from matching a point like b in the diagram and ensures that
   // c is contained within the resulting box.

#if MUSHSPACE_DIM == 1
   (void)space; (void)c; (void)last; (void)aabb;
   return false;
#else
   bool positive;
   mushdim axis = MUSHSPACE_DIM;

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
      if (axis == MUSHSPACE_DIM) {
         if (c.v[i] >  space->first_placed_big.v[i]
          && c.v[i] <= space->first_placed_big.v[i] + BIG_SEQ_MAX_SPACING)
         {
            positive = true;
            axis = i;
            continue;
         }
         if (c.v[i] <  space->first_placed_big.v[i]
          && c.v[i] >= space->first_placed_big.v[i] - BIG_SEQ_MAX_SPACING)
         {
            positive = false;
            axis = i;
            continue;
         }
      }
      if (c.v[i] != space->first_placed_big.v[i])
         return false;
   }
   if (axis == MUSHSPACE_DIM)
      return false;

   mushbounds bounds;
   if (positive) {
      bounds.beg = space->big_sequence_start;
      bounds.end = last->end;

      mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
   } else {
      bounds.beg = last->beg;
      bounds.end = space->big_sequence_start;

      mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);
   }
   mushaabb_make_unsafe(aabb, &bounds);
   return true;
#endif
}
