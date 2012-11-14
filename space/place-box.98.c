// File created: 2012-01-27 22:57:55

#include "space/place-box.98.h"

#include <alloca.h>
#include <assert.h>

#include "aabb/consume.98.h"
#include "aabb/subsume.98.h"
#include "aabb/space-area.98.h"
#include "space/heuristic-constants.98.h"

typedef struct consumee {
   mushboxen_iter iter;
   size_t size;
   void* iter_aux;
} consumee;

static mushboxen_iter really_place_box(mushspace*, mushaabb*, void* aux);

static bool subsume_fusables(mushspace*, mushbounds*, consumee*, size_t*);
static bool subsume_disjoint(mushspace*, mushbounds*, consumee*, size_t*);
static bool subsume_overlaps(mushspace*, mushbounds*, consumee*, size_t*);

static bool disjoint_mms_validator(
   const mushbounds*, const mushaabb*, size_t, void*);

static bool overlaps_mms_validator(
   const mushbounds*, const mushaabb*, size_t, void*);

static void min_max_size(mushbounds*, consumee*, size_t*, mushboxen_iter);

static bool valid_min_max_size(
   bool (*)(const mushbounds*, const mushaabb*, size_t, void*), void*,
   mushbounds*, consumee*, size_t*, mushboxen_iter);

static bool cheaper_to_alloc(size_t, size_t);

static bool consume_and_subsume(mushspace*, mushboxen_iter, mushaabb*);

int mushspace_place_box(
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

   bool invalidate = false,
        success    = true;

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));

#if USE_BAKAABB
   void *aux2 = alloca(mushboxen_iter_aux_size(&space->boxen));
#endif

   // Then do the actual placement.
   for (size_t b = 0; b < a; ++b) {
      mushaabb   *box    = &aabbs[b];
      mushbounds *bounds = &box->bounds;

      if ((mushstaticaabb_contains(bounds->beg)
        && mushstaticaabb_contains(bounds->end))
       || mushboxen_contains_bounds(&space->boxen, bounds))
      {
         mushstats_add(space->stats, MushStat_boxes_incorporated, 1);
         continue;
      }

      mushaabb_finalize(box);
      mushboxen_iter iter = really_place_box(space, box, aux);
      if (mushboxen_iter_is_null(iter)) {
         success = false;
         break;
      }

      invalidate = true;

      bounds = &box->bounds;

      if (reason && mushbounds_contains(bounds, *reason)) {
         if (b == a-1) {
            // We won't be calling really_place_box any more so we can safely
            // return the reason box.
            *reason_box = mushboxen_iter_box(iter);
         } else {
            // Later calls to really_place_box may invalidate the reason box,
            // so we can't return it.
            *reason_box = NULL;
         }

         // This can only happen once.
         reason = NULL;
      }

#if USE_BAKAABB
      // If it crossed bak, we need to fix things up and move any occupied
      // cells from bak (which is below all boxen) to the appropriate box.
      // (which may not be *box, if there are any boxes above it).
      if (!space->bak.data || !mushbakaabb_size(&space->bak))
         continue;

      if (!mushbounds_overlaps(bounds, &space->bak.bounds))
         continue;

      const bool overlaps =
         !mushboxen_iter_above_done(
            mushboxen_iter_above_init(&space->boxen, iter, aux2),
            &space->boxen);

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
#endif
   }
   if (invalidate && !mushspace_invalidate_all(space) && success)
      return MUSHERR_INVALIDATION_FAILURE;
   if (!success)
      return MUSHERR_OOM;
   return MUSHERR_NONE;
}

// Returns the placed box, which may be bigger than the given box. Returns the
// null iterator if memory allocation failed.
static mushboxen_iter really_place_box(
   mushspace* space, mushaabb* aabb, void* external_aux)
{
#ifdef MUSH_ENABLE_EXPENSIVE_DEBUGGING
   assert (!mushboxen_contains_bounds(&space->boxen, &aabb->bounds));
#endif

   consumee consumee = {.size = 0, .iter_aux = external_aux};
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
   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_in it =
           mushboxen_iter_in_init(&space->boxen, &consumer.bounds, aux);
        !mushboxen_iter_in_done( it, &space->boxen);
         mushboxen_iter_in_next(&it, &space->boxen))
   {
      const mushaabb *box = mushboxen_iter_in_box(it);
      any_subsumees = true;
      if (box->size > consumee.size) {
         consumee.size = box->size;
         consumee.iter =
            mushboxen_iter_copy(*(mushboxen_iter*)&it, consumee.iter_aux);
      }
   }

   mushboxen_iter inserted;
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
      mushboxen_iter consumee_it = consumee.iter;
      mushboxen_reservation reserved;
      if (!mushboxen_reserve_preserve(&space->boxen, &reserved, &consumee_it))
         return mushboxen_iter_null;

      const bool ok = consume_and_subsume(space, consumee_it, &consumer);
      if (!ok) {
         mushboxen_unreserve(&space->boxen, &reserved);
         return mushboxen_iter_null;
      }

      inserted = mushboxen_insert_reservation(
         &space->boxen, &reserved, &consumer, external_aux);

      *aabb = consumer;
   } else {
      // When we have nothing to consume, things are simpler: just allocate the
      // given AABB, make room for it, and add it.

      if (!mushaabb_alloc(aabb))
         return mushboxen_iter_null;

      inserted = mushboxen_insert(&space->boxen, aabb, external_aux);
      if (mushboxen_iter_is_null(inserted)) {
         free(aabb->data);
         return inserted;
      }
   }

   mushstats_add    (space->stats, MushStat_boxes_placed, 1);
   mushstats_new_max(space->stats, MushStat_max_boxes_live,
                     (uint64_t)mushboxen_count(&space->boxen));

   return inserted;
}

static bool subsume_fusables(
   mushspace* space, mushbounds* consumer,
   consumee* consumee, size_t* used_cells)
{
   // All fusables must lie immediately next to consumer, so looking for those
   // which overlap with a copy of the consumer expanded by 1 cell in each
   // direction will get all candidates.
   mushbounds exp_consumer;

   #define UPDATE_EXP(cons) do { \
      exp_consumer.beg = mushcoords_subs_clamped((cons)->beg, 1); \
      exp_consumer.end = mushcoords_adds_clamped((cons)->end, 1); \
   } while (0)

   #define UPDATE_ITER(cons) do { \
      UPDATE_EXP(cons); \
      mushboxen_iter_overout_updated_next(&it, &space->boxen); \
   } while (0)

   mushboxen_iter_overout it;

   UPDATE_EXP(consumer);

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));

#if MUSHSPACE_DIM == 1
   // The one-dimensional case is simple, as we don't have to worry about axes.

   it =
      mushboxen_iter_overout_init(&space->boxen, &exp_consumer, consumer, aux);

   bool any = false;
   while (!mushboxen_iter_overout_done(it, &space->boxen)) {
      const mushbounds *bounds = &mushboxen_iter_overout_box(it)->bounds;
      if (!mushbounds_can_fuse(consumer, bounds)) {
         mushboxen_iter_overout_next(&it, &space->boxen);
         continue;
      }

      min_max_size(consumer, consumee, used_cells, *(mushboxen_iter*)&it);
      UPDATE_ITER(consumer);
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

   it = mushboxen_iter_overout_init(
      &space->boxen, &exp_consumer, &tentative_consumer, aux);

   for (;; mushboxen_iter_overout_next(&it, &space->boxen)) {
next:
      if (mushboxen_iter_overout_done(it, &space->boxen))
         break;

      const mushbounds *bounds = &mushboxen_iter_overout_box(it)->bounds;
      if (!mushbounds_can_fuse(&tentative_consumer, bounds))
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
            *(mushboxen_iter*)&it);
         UPDATE_ITER(&tentative_consumer);
         ++n;
         goto next;
      }

      // Try all axes starting from the primary one.
      for (mushdim x = MUSHSPACE_DIM; x--;) {
         if (!mushbounds_on_same_axis(&tentative_consumer, bounds, x))
            continue;

         min_max_size(
            &tentative_consumer, &tentative_consumee, &tentative_used_cells,
            *(mushboxen_iter*)&it);
         UPDATE_ITER(&tentative_consumer);
         ++n;
         axis = x;
         goto next;
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

   // All fusables have been removed, so a sufficient condition for
   // disjointness is non-overlappingness. This also takes care of excluding
   // the subsumees, which are contained and hence overlapping.
   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_out it =
           mushboxen_iter_out_init(&space->boxen, consumer, aux);
        !mushboxen_iter_out_done( it, &space->boxen);)
   {
      if (!valid_min_max_size(disjoint_mms_validator, NULL, consumer, consumee,
                              used_cells, *(mushboxen_iter*)&it))
      {
         mushboxen_iter_out_next(&it, &space->boxen);
         continue;
      }

      mushboxen_iter_out_updated_next(&it, &space->boxen);
      mushstats_add(space->stats, MushStat_subsumed_disjoint, 1);
      any = true;
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
   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_overout it =
           mushboxen_iter_overout_init(&space->boxen, consumer, consumer, aux);
        !mushboxen_iter_overout_done(it, &space->boxen);)
   {
      if (!valid_min_max_size(overlaps_mms_validator, consumer, consumer,
                              consumee, used_cells, *(mushboxen_iter*)&it))
      {
         mushboxen_iter_overout_next(&it, &space->boxen);
         continue;
      }

      mushboxen_iter_overout_updated_next(&it, &space->boxen);
      mushstats_add(space->stats, MushStat_subsumed_overlaps, 1);
      any = true;
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
   mushbounds* bounds, consumee* max, size_t* total_size, mushboxen_iter it)
{
   const mushaabb *box = mushboxen_iter_box(it);

   assert (!mushbounds_contains_bounds(bounds, &box->bounds));

   *total_size += box->size;
   if (box->size > max->size) {
      max->size = box->size;
      max->iter = mushboxen_iter_copy(it, max->iter_aux);
   }
   mushbounds_expand_to_cover(bounds, &box->bounds);
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
   mushboxen_iter it)
{
   mushbounds try_bounds = *bounds;
   consumee try_max = *max;
   size_t try_total_size = *total_size;

   min_max_size(&try_bounds, &try_max, &try_total_size, it);

   if (!valid(&try_bounds, mushboxen_iter_box(it), *total_size, userdata))
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
   mushspace* space, mushboxen_iter consumee, mushaabb* consumer)
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
   void *aux  = alloca(mushboxen_iter_aux_size(&space->boxen)),
        *aux2 = alloca(mushboxen_iter_aux_size(&space->boxen));
   for (mushboxen_iter_in hit =
           mushboxen_iter_in_init(&space->boxen, &consumer->bounds, aux);
        !mushboxen_iter_in_done( hit, &space->boxen);
         mushboxen_iter_in_next(&hit, &space->boxen))
   {
      const mushaabb *higher = mushboxen_iter_in_box(hit);

      for (mushboxen_iter_below lit =
              mushboxen_iter_below_init(
                 &space->boxen, *(mushboxen_iter*)&hit, aux2);
           !mushboxen_iter_below_done( lit, &space->boxen);
            mushboxen_iter_below_next(&lit, &space->boxen))
      {
         mushaabb *lower = mushboxen_iter_below_box(lit);

         // We will subsume bottom-up, so if the lower box is a subsumee no
         // copying is necessary — unless the higher box is the consumee. The
         // consumee is taken care of out of order, so it always needs to be
         // handled here as well.
         if (higher->data != mushboxen_iter_box(consumee)->data
          && mushbounds_contains_bounds(&consumer->bounds, &lower->bounds))
            continue;

         mushaabb overlap;

         // Copy the overlap area to the lower box.
         mushbounds_get_overlap(&higher->bounds, &lower->bounds,
                                &overlap.bounds);
         mushaabb_finalize(&overlap);
         mushaabb_subsume_area(lower, higher, &overlap);
      }
   }

   mushaabb_finalize(consumer);
   if (!mushaabb_consume(consumer, mushboxen_iter_box(consumee)))
      return false;

   // So that we don't try to free it when removing below: it's already been
   // realloced.
   mushboxen_iter_box(consumee)->data = NULL;

   mushboxen_remsched rs =
      mushboxen_remsched_init(&space->boxen, consumee, aux);

   // Subsume lower boxes first: we want higher boxes to overwrite lower ones,
   // not the other way around.
   for (mushboxen_iter_in_bottomup it =
           mushboxen_iter_in_bottomup_init(
              &space->boxen, &consumer->bounds, aux2);
        !mushboxen_iter_in_bottomup_done(it, &space->boxen);)
   {
      mushaabb *box = mushboxen_iter_in_bottomup_box(it);
      if (!box->data) {
         // It must be the consumee.
         mushboxen_iter_in_bottomup_next(&it, &space->boxen);
         continue;
      }
      mushaabb_subsume(consumer, box);
      mushboxen_iter_in_bottomup_sched_remove(&it, &space->boxen, &rs);
   }
   mushboxen_remsched_apply(&space->boxen, &rs);
   return true;
}
