// File created: 2012-01-27 22:57:55

#include "space/place-box.98.h"

#include <assert.h>

#include "aabb/consume.98.h"
#include "aabb/subsume.98.h"
#include "aabb/space-area.98.h"
#include "space/heuristic-constants.98.h"

typedef struct consumee { size_t idx; size_t size; } consumee;

static mushaabb* really_place_box(mushspace*, mushaabb*);

static bool subsume_fusables(mushspace*, mushbounds*, consumee*, size_t*);
static bool subsume_disjoint(mushspace*, mushbounds*, consumee*, size_t*);
static bool subsume_overlaps(mushspace*, mushbounds*, consumee*, size_t*);

static bool disjoint_mms_validator(
   const mushbounds*, const mushaabb*, size_t, void*);

static bool overlaps_mms_validator(
   const mushbounds*, const mushaabb*, size_t, void*);

static void min_max_size(mushbounds*, consumee*, size_t*, mushcaabb_idx);

static bool valid_min_max_size(
   bool (*)(const mushbounds*, const mushaabb*, size_t, void*), void*,
   mushbounds*, consumee*, size_t*, mushcaabb_idx);

static bool cheaper_to_alloc(size_t, size_t);

static bool consume_and_subsume(mushspace*, size_t, mushaabb*);

bool mushspace_place_box(
   mushspace* space, mushaabb* aabb, mushcoords* reason, mushaabb** reason_box)
{
   assert ((reason == NULL) == (reason_box == NULL));
   if (reason)
      assert (mushbounds_safe_contains(&aabb->bounds, *reason));

   // Split the box up along any axes it wraps around on.
   mushaabb aabbs[1 << MUSHSPACE_DIM];
   aabbs[0] = *aabb;
   size_t a = 1;

   for (size_t b = 0; b < a; ++b) {
      mushbounds *bounds = &aabbs[b].bounds;
      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
         if (bounds->beg.v[i] <= bounds->end.v[i])
            continue;

         mushbounds clipped = *bounds;
         clipped.end.v[i] = MUSHCELL_MAX;
         mushaabb_make_unsafe(&aabbs[a++], &clipped);
         bounds->beg.v[i] = MUSHCELL_MIN;
      }
   }

   // Then do the actual placement.
   for (size_t b = 0; b < a; ++b) {
      mushaabb   *box    = &aabbs[b];
      mushbounds *bounds = &box->bounds;

      if (mushstaticaabb_contains(bounds->beg)
       && mushstaticaabb_contains(bounds->end))
      {
incorporated:
         mushstats_add(space->stats, MushStat_boxes_incorporated, 1);
         continue;
      }

      for (size_t i = 0; i < space->box_count; ++i)
         if (mushbounds_contains_bounds(&space->boxen[i].bounds, bounds))
            goto incorporated;

      mushaabb_finalize(box);
      box = really_place_box(space, box);
      if (box == NULL)
         return false;

      bounds = &box->bounds;

      if (reason && mushbounds_contains(bounds, *reason)) {
         *reason_box = box;

         // This can only happen once.
         reason = NULL;
      }

      // If it crossed bak, we need to fix things up and move any occupied
      // cells from bak (which is below all boxen) to the appropriate box
      // (which may not be *box, if it has any overlaps).
      if (!space->bak.data || !mushbakaabb_size(&space->bak))
         continue;

      if (!mushbounds_overlaps(bounds, &space->bak.bounds))
         continue;

      assert (box == &space->boxen[space->box_count-1]);

      bool overlaps = false;
      for (size_t b = 0; b < space->box_count-1; ++b) {
         if (mushbounds_overlaps(bounds, &space->boxen[b].bounds)) {
            overlaps = true;
            break;
         }
      }

      unsigned char buf[mushbakaabb_iter_sizeof];
      mushbakaabb_iter *it = mushbakaabb_it_start(&space->bak, buf);

      for (; !mushbakaabb_it_done(it, &space->bak);
              mushbakaabb_it_next(it, &space->bak))
      {
         mushcoords c = mushbakaabb_it_pos(it, &space->bak);
         if (!mushbounds_contains(bounds, c))
            continue;

         mushcell v = mushbakaabb_it_val(it, &space->bak);

         mushbakaabb_it_remove(it, &space->bak);

         if (overlaps)
            mushspace_put(space, c, v);
         else
            mushaabb_put(box, c, v);
      }
      mushbakaabb_it_stop(it);
   }
   return true;
}

// Returns the placed box, which may be bigger than the given box. Returns NULL
// if memory allocation failed.
static mushaabb* really_place_box(mushspace* space, mushaabb* aabb) {
   for (size_t i = 0; i < space->box_count; ++i)
      assert (!mushbounds_contains_bounds(
         &space->boxen[i].bounds, &aabb->bounds));

   consumee consumee = {.size = 0};
   size_t used_cells = aabb->size;

   mushaabb consumer;
   mushaabb_make_unsafe(&consumer, &aabb->bounds);

   bool any_subsumees = false;

   for (;;) {
      // Disjoint assumes that it comes after fusables. Some reasoning for why
      // that's probably a good idea anyway:
      //
      // F
      // FD
      // A
      //
      // F is a fusable, D is a disjoint, and A is *aabb. If we looked for
      // disjoints before fusables, we might subsume D, leaving us worse off
      // than if we'd subsumed F.

      #define PARAMS space, &consumer.bounds, &consumee, &used_cells

      if (subsume_fusables(PARAMS)) { any_subsumees = true; }
      if (subsume_disjoint(PARAMS)) { any_subsumees = true; continue; }
      if (subsume_overlaps(PARAMS)) { any_subsumees = true; continue; }
      break;

      #undef PARAMS
   }

   // Look for contained boxes in case any_subsumees in false. Do it in any
   // case since we might improve consumee.
   //
   // Disjoints and overlaps can bring in new contained boxes that weren't
   // handled at all before this, so we do need to do this here.
   for (size_t i = 0; i < space->box_count; ++i) {
      const mushaabb *box = &space->boxen[i];
      if (!mushbounds_contains_bounds(&consumer.bounds, &box->bounds))
         continue;
      any_subsumees = true;
      if (box->size > consumee.size)
         consumee = (struct consumee){.size = box->size, .idx = i};
   }

   if (any_subsumees) {
      // Even though consume_and_subsume might reduce the box count and
      // generally free some memory (by defragmentation if nothing else), that
      // doesn't necessarily happen. (Due to other programs if nothing else.)
      //
      // So in the worst case, we do need one more AABB's worth of memory. And
      // in that case, if we do the allocation after the consume_and_subsume
      // and it fails, we're screwed: we've eaten the consumee but can't place
      // the consumer. Thus we must preallocate here even if it ends up being
      // unnecessary.
      size_t max_needed_boxen = space->box_count + 1;
      mushaabb *boxen =
         realloc(space->boxen, max_needed_boxen * sizeof *boxen);
      if (!boxen)
         return NULL;
      space->boxen = boxen;

      const bool ok = consume_and_subsume(space, consumee.idx, &consumer);

      // Try to reduce size of space->boxen if possible.
      //
      // In the ok case, consume_and_subsume may have reduced box_count, and in
      // the non-ok case, we've extended the size of boxen by 1 uselessly,
      // which we should try to undo.

      const size_t reduce_to_size = space->box_count + (ok ? 1 : 0);

      if (reduce_to_size < max_needed_boxen) {
         boxen = realloc(space->boxen, reduce_to_size * sizeof *boxen);
         if (boxen)
            space->boxen = boxen;
      }

      if (!ok)
         return NULL;

      space->boxen[space->box_count++] = consumer;
   } else {
      // When we have nothing to consume, things are simpler: just allocate the
      // given AABB, make room for it, and add it.

      if (!mushaabb_alloc(aabb))
         return NULL;

      mushaabb *boxen =
         realloc(space->boxen, (space->box_count+1) * sizeof *boxen);
      if (!boxen) {
         free(aabb->data);
         return NULL;
      }

      (space->boxen = boxen)[space->box_count++] = *aabb;
   }

   mushstats_add    (space->stats, MushStat_boxes_placed, 1);
   mushstats_new_max(space->stats, MushStat_max_boxes_live,
                     (uint64_t)space->box_count);

   mushspace_invalidate_all(space);

   return &space->boxen[space->box_count-1];
}

static bool subsume_fusables(
   mushspace* space, mushbounds* consumer,
   consumee* consumee, size_t* used_cells)
{
#if MUSHSPACE_DIM == 1
   // The one-dimensional case is simple, as we don't have to worry about axes.

   bool any = false;
   for (size_t c = 0; c < space->box_count; ++c) {
      const mushbounds *bounds = &space->boxen[c].bounds;
      if (!mushbounds_can_fuse(consumer, bounds)
       || mushbounds_contains_bounds(consumer, bounds))
         continue;

      min_max_size(consumer, consumee, used_cells,
                   mushspace_get_caabb_idx(space, c));
      mushstats_add(space->stats, MushStat_subsumed_fusables, 1);
      any = true;
   }
   return any;
#else
   // We need to ensure that all the ones we fuse with are along the same axis.
   // For instance, A can't fuse with both X and Y in the following:
   //
   // X
   // AY
   //
   // We prefer those along the primary axis (y for 2D, z for 3D) because then
   // we have less memcpying/memmoving to do.

   static const mushdim PRIMARY_AXIS = MUSHSPACE_DIM - 1;
   mushdim axis = MUSHSPACE_DIM;

   mushbounds      tentative_consumer   = *consumer;
   struct consumee tentative_consumee   = *consumee;
   size_t          tentative_used_cells = *used_cells;

   size_t n = 0;

   for (size_t c = 0; c < space->box_count; ++c) {
      const mushbounds *bounds = &space->boxen[c].bounds;
      if (!mushbounds_can_fuse(&tentative_consumer, bounds)
        || mushbounds_contains_bounds(&tentative_consumer, bounds))
         continue;

      if (axis != MUSHSPACE_DIM) {
         if (axis != PRIMARY_AXIS
          && mushbounds_on_same_axis(
                &tentative_consumer, bounds, PRIMARY_AXIS))
         {
            axis = PRIMARY_AXIS;

            // We've tentatively fused with some non-primary bounds: reset.
            tentative_consumer   = *consumer;
            tentative_consumee   = *consumee;
            tentative_used_cells = *used_cells;
            n = 0;
         } else if (!mushbounds_on_same_axis(
                        &tentative_consumer, bounds, axis))
            continue;

         min_max_size(
            &tentative_consumer, &tentative_consumee, &tentative_used_cells,
            mushspace_get_caabb_idx(space, c));
         ++n;
         continue;
      }

      // Try all axes starting from the primary one.
      for (mushdim x = MUSHSPACE_DIM; x--;) {
         if (!mushbounds_on_same_axis(&tentative_consumer, bounds, x))
            continue;

         min_max_size(
            &tentative_consumer, &tentative_consumee, &tentative_used_cells,
            mushspace_get_caabb_idx(space, c));
         ++n;
         axis = x;
         break;
      }
   }
   if (n) {
      *consumer   = tentative_consumer;
      *consumee   = tentative_consumee;
      *used_cells = tentative_used_cells;
      mushstats_add(space->stats, MushStat_subsumed_fusables, n);
   }
   return n;
#endif
}

static bool subsume_disjoint(
   mushspace* space, mushbounds* consumer,
   consumee* consumee, size_t* used_cells)
{
   bool any = false;
   for (size_t c = 0; c < space->box_count; ++c) {
      mushcaabb_idx box = mushspace_get_caabb_idx(space, c);

      // All fusables have been removed, so a sufficient condition for
      // disjointness is non-overlappingness. This also takes care of excluding
      // the subsumees, which are contained and hence overlapping.
      if (mushbounds_overlaps(consumer, &box.aabb->bounds))
         continue;

      if (valid_min_max_size(disjoint_mms_validator, NULL, consumer, consumee,
                             used_cells, box))
      {
         mushstats_add(space->stats, MushStat_subsumed_disjoint, 1);
         any = true;
      }
   }
   return any;
}

static bool disjoint_mms_validator(
   const mushbounds* b, const mushaabb* fodder, size_t used_cells, void* nil)
{
   (void)nil;
   return cheaper_to_alloc(
      mushbounds_clamped_size(b), used_cells + fodder->size);
}

static bool subsume_overlaps(
   mushspace* space, mushbounds* consumer,
   consumee* consumee, size_t* used_cells)
{
   bool any = false;
   for (size_t c = 0; c < space->box_count; ++c) {
      mushcaabb_idx box = mushspace_get_caabb_idx(space, c);

      const mushbounds *bounds = &box.aabb->bounds;
      if (mushbounds_contains_bounds(consumer, bounds)
       || !mushbounds_overlaps(consumer, bounds))
         continue;

      if (valid_min_max_size(overlaps_mms_validator, consumer, consumer,
                             consumee, used_cells, box))
      {
         mushstats_add(space->stats, MushStat_subsumed_overlaps, 1);
         any = true;
      }
   }
   return any;
}

static bool overlaps_mms_validator(
   const mushbounds* b, const mushaabb* fodder, size_t used_cells, void* cp)
{
   const mushbounds *consumer = cp;

   mushaabb overlap;
   mushbounds_get_overlap(consumer, &fodder->bounds, &overlap.bounds);
   mushaabb_finalize(&overlap);

   return cheaper_to_alloc(
      mushbounds_clamped_size(b), used_cells + fodder->size - overlap.size);
}

static void min_max_size(
   mushbounds* bounds, consumee* max, size_t* total_size, mushcaabb_idx box)
{
   assert (!mushbounds_contains_bounds(bounds, &box.aabb->bounds));

   *total_size += box.aabb->size;
   if (box.aabb->size > max->size) {
      max->size = box.aabb->size;
      max->idx  = box.idx;
   }
   mushcoords_min_into(&bounds->beg, box.aabb->bounds.beg);
   mushcoords_max_into(&bounds->end, box.aabb->bounds.end);
}

// Fills in the input values with the min_max_size data, returning what the
// given validator function returns.
//
// The validator takes:
// - bounds of the box that subsumes
// - box to be subsumed (allocated)
// - number of cells that are currently contained in any box that the subsumer
//   contains
// - arbitrary user-provided data
static bool valid_min_max_size(
   bool (*valid)(const mushbounds*, const mushaabb*, size_t, void*),
   void* userdata,
   mushbounds* bounds,
   consumee* max, size_t* total_size,
   mushcaabb_idx box)
{
   mushbounds try_bounds = *bounds;
   consumee try_max = *max;
   size_t try_total_size = *total_size;

   min_max_size(&try_bounds, &try_max, &try_total_size, box);

   if (!valid(&try_bounds, box.aabb, *total_size, userdata))
      return false;

   *bounds     = try_bounds;
   *max        = try_max;
   *total_size = try_total_size;
   return true;
}

static bool cheaper_to_alloc(size_t together, size_t separate) {
   return together <= ACCEPTABLE_WASTE
       ||   sizeof(mushcell) * (together - ACCEPTABLE_WASTE)
          < sizeof(mushcell) * separate + sizeof(mushaabb);
}

static bool consume_and_subsume(
   mushspace* space, size_t consumee, mushaabb* consumer)
{
   // Consider the following:
   //
   // +-----++---+
   // | A +--| C |
   // +---|B +*--+
   //     +----+
   //
   // Here, A is the one being placed and C is a fusable. * is a point whose
   // data is in C but which is contained in both B and C. Since consumer is
   // going to end up below all existing boxes, we'll be left with:
   //
   // +----------+
   // | X +----+ |
   // +---|B  *|-+
   //     +----+
   //
   // Where X is the consumer. Note that * is now found in B, not in X, but its
   // data was in C (now X)! Oops!
   //
   // So, we do this, which in the above case would copy the data from C to B.
   //
   // We need to copy data from each subsumee S to all boxes that are currently
   // below S and which will end up above the consumer.
   for (size_t s = 0; s < space->box_count; ++s) {
      const mushaabb* higher = &space->boxen[s];
      if (!mushbounds_contains_bounds(&consumer->bounds, &higher->bounds))
         continue;

      for (size_t t = s+1; t < space->box_count; ++t) {
         mushaabb* lower = &space->boxen[t];

         // We will subsume bottom-up, so if the lower box is a subsumee no
         // copying is necessary — unless the higher box is the consumee. The
         // consumee is taken care of out of order, so it always needs to be
         // handled here as well.
         if (s != consumee
          && mushbounds_contains_bounds(&consumer->bounds, &lower->bounds))
            continue;

         mushaabb overlap;

         // If they overlap, copy the overlap area to the lower box.
         if (mushbounds_get_overlap(&higher->bounds, &lower->bounds,
                                    &overlap.bounds))
         {
            mushaabb_finalize(&overlap);
            mushaabb_subsume_area(lower, higher, &overlap);
         }
      }
   }

   mushaabb_finalize(consumer);
   if (!mushaabb_consume(consumer, &space->boxen[consumee]))
      return false;

   // So that remove_boxes doesn't try to free it: it's already been realloced.
   space->boxen[consumee].data = NULL;

   size_t range_beg = consumee, range_end = range_beg;

   // Subsume lower boxes first: we want higher boxes to overwrite lower ones,
   // not the other way around.
   for (size_t b = space->box_count; b--;) {
      if (b == consumee)
         continue;

      mushaabb *box = &space->boxen[b];
      if (!mushbounds_contains_bounds(&consumer->bounds, &box->bounds))
         continue;

      mushaabb_subsume(consumer, box);

      if (b == range_beg - 1) { range_beg = b; continue; }
      if (b == range_end + 1) { range_end = b; continue; }

      mushspace_remove_boxes(space, range_beg, range_end);

      if (range_end < b) {
         // This should only happen when the consumee was off to the left.
         assert (range_beg == consumee);
         assert (range_end == consumee);
         --b;
      }

      range_beg = range_end = b;
   }
   mushspace_remove_boxes(space, range_beg, range_end);
   return true;
}
