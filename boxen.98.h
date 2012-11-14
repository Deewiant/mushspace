// File created: 2012-11-03 21:02:39

#ifndef MUSHSPACE_BOXEN_H
#define MUSHSPACE_BOXEN_H

#include <stdbool.h>
#include <stdlib.h>

#include "aabb.98.h"
#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mushboxen                  MUSHSPACE_NAME(mushboxen)
#define mushboxen_iter             MUSHSPACE_CAT(mushboxen,_iter)
#define mushboxen_iter_above       MUSHSPACE_CAT(mushboxen,_iter_above)
#define mushboxen_iter_below       MUSHSPACE_CAT(mushboxen,_iter_below)
#define mushboxen_iter_in          MUSHSPACE_CAT(mushboxen,_iter_in)
#define mushboxen_iter_in_bottomup MUSHSPACE_CAT(mushboxen,_iter_in_bottomup)
#define mushboxen_iter_out         MUSHSPACE_CAT(mushboxen,_iter_out)

#define mushboxen_init              MUSHSPACE_CAT(mushboxen,_init)
#define mushboxen_count             MUSHSPACE_CAT(mushboxen,_count)
#define mushboxen_get               MUSHSPACE_CAT(mushboxen,_get)
#define mushboxen_get_iter          MUSHSPACE_CAT(mushboxen,_get_iter)
#define mushboxen_contains_bounds   MUSHSPACE_CAT(mushboxen,_contains_bounds)
#define mushboxen_reservation       MUSHSPACE_CAT(mushboxen,_reservation)
#define mushboxen_reserve           MUSHSPACE_CAT(mushboxen,_reserve)
#define mushboxen_unreserve         MUSHSPACE_CAT(mushboxen,_unreserve)
#define mushboxen_iter_aux_size     MUSHSPACE_CAT(mushboxen,_iter_aux_size)
#define mushboxen_iter_init         MUSHSPACE_CAT(mushboxen,_iter_init)
#define mushboxen_iter_done         MUSHSPACE_CAT(mushboxen,_iter_done)
#define mushboxen_iter_next         MUSHSPACE_CAT(mushboxen,_iter_next)
#define mushboxen_iter_above_init   MUSHSPACE_CAT(mushboxen,_iter_above_init)
#define mushboxen_iter_above_done   MUSHSPACE_CAT(mushboxen,_iter_above_done)
#define mushboxen_iter_above_next   MUSHSPACE_CAT(mushboxen,_iter_above_next)
#define mushboxen_iter_below_init   MUSHSPACE_CAT(mushboxen,_iter_below_init)
#define mushboxen_iter_below_done   MUSHSPACE_CAT(mushboxen,_iter_below_done)
#define mushboxen_iter_below_next   MUSHSPACE_CAT(mushboxen,_iter_below_next)
#define mushboxen_iter_in_init      MUSHSPACE_CAT(mushboxen,_iter_in_init)
#define mushboxen_iter_in_done      MUSHSPACE_CAT(mushboxen,_iter_in_done)
#define mushboxen_iter_in_next      MUSHSPACE_CAT(mushboxen,_iter_in_next)
#define mushboxen_iter_overout_init MUSHSPACE_CAT(mushboxen,_iter_overout_init)
#define mushboxen_iter_overout_done MUSHSPACE_CAT(mushboxen,_iter_overout_done)
#define mushboxen_iter_overout_next MUSHSPACE_CAT(mushboxen,_iter_overout_next)
#define mushboxen_iter_out_init     MUSHSPACE_CAT(mushboxen,_iter_out_init)
#define mushboxen_iter_out_done     MUSHSPACE_CAT(mushboxen,_iter_out_done)
#define mushboxen_iter_out_next     MUSHSPACE_CAT(mushboxen,_iter_out_next)
#define mushboxen_iter_box          MUSHSPACE_CAT(mushboxen,_iter_box)
#define mushboxen_iter_null         MUSHSPACE_CAT(mushboxen,_iter_null)
#define mushboxen_iter_is_null      MUSHSPACE_CAT(mushboxen,_iter_is_null)
#define mushboxen_iter_remove       MUSHSPACE_CAT(mushboxen,_iter_remove)
#define mushboxen_remsched_init     MUSHSPACE_CAT(mushboxen,_remsched_init)
#define mushboxen_remsched_apply    MUSHSPACE_CAT(mushboxen,_remsched_apply)
#define mushboxen_iter_in_bottomup_init \
   MUSHSPACE_CAT(mushboxen,_iter_in_bottomup_init)
#define mushboxen_iter_in_bottomup_done \
   MUSHSPACE_CAT(mushboxen,_iter_in_bottomup_done)
#define mushboxen_iter_in_bottomup_next \
   MUSHSPACE_CAT(mushboxen,_iter_in_bottomup_next)
#define mushboxen_iter_in_bottomup_sched_remove \
   MUSHSPACE_CAT(mushboxen,_iter_in_bottomup_sched_remove)
#define mushboxen_insert_reservation \
   MUSHSPACE_CAT(mushboxen,_insert_reservation)
#define mushboxen_iter_out_updated_next \
   MUSHSPACE_CAT(mushboxen,_iter_out_updated_next)
#define mushboxen_iter_overout_updated_next \
   MUSHSPACE_CAT(mushboxen,_iter_overout_updated_next)

// Internals are visible so that we can store mushboxen directly in mushspace,
// and for the rest, because it's convenient to not mess around with buffers.

#include "boxen/array.h"

///// Basic API

// mushboxen obeys an invariant called /T-ordering/ (T for time). What
// T-ordering means is that if two boxes A and B overlap, and A was inserted
// prior to B, then mushboxen_get for any overlapping coordinate always returns
// box A. In this case, A is /T-above/ B.
//
// T-ordering between nonoverlapping boxes is meaningless: if A and B do not
// overlap, neither is T-above the other.

void           mushboxen_init    (mushboxen*);
void           mushboxen_free    (mushboxen*);
bool           mushboxen_copy    (mushboxen*, const mushboxen*);
size_t         mushboxen_count   (const mushboxen*);
mushaabb*      mushboxen_get     (const mushboxen*, mushcoords);
mushboxen_iter mushboxen_get_iter(const mushboxen*, mushcoords, void* aux);
mushboxen_iter mushboxen_insert  (mushboxen*, mushaabb*, void* aux);

bool mushboxen_contains_bounds(const mushboxen*, const mushbounds*);

void mushboxen_loosen_bounds(const mushboxen*, mushbounds*);

///// Reservation API

// Preserves the given iterator's validity, possibly modifying it in doing so.
bool mushboxen_reserve_preserve(
   mushboxen*, mushboxen_reservation*, mushboxen_iter*);

void mushboxen_unreserve(mushboxen*, mushboxen_reservation*);

mushboxen_iter mushboxen_insert_reservation(
   mushboxen*, mushboxen_reservation*, mushaabb*, void* aux);

///// Iterator API

// This should be a small, allocaäble size. Typically used to emulate
// recursion, since the call stack isn't available due to using iterators.
//
// The size should never grow when boxes are removed.
size_t mushboxen_iter_aux_size(const mushboxen*);

// When alloca isn't available, this can be used to malloc something initially.
// The idea being that this should be big enough for most uses.
//
// Warning: can be zero.
extern const size_t mushboxen_iter_aux_size_init;

// It should always be safe to cast any mushboxen_iter_FOO* to const
// mushboxen_iter*.

// Iterator creation functions

mushboxen_iter mushboxen_iter_init(const mushboxen*, void* aux);
mushboxen_iter mushboxen_iter_copy(mushboxen_iter, void* aux);

// These refer to T-ordering.
mushboxen_iter_above mushboxen_iter_above_init(
   const mushboxen*, mushboxen_iter, void* aux);
mushboxen_iter_below mushboxen_iter_below_init(
   const mushboxen*, mushboxen_iter, void* aux);

// This and all the other bounds-based iterators refer to the bounds by
// reference! If it changes, the appropriate _updated function should be
// called.
mushboxen_iter_in mushboxen_iter_in_init(
   const mushboxen*, const mushbounds*, void* aux);

// Like mushboxen_iter_in, but guarantees bottom-up traversal order.
mushboxen_iter_in_bottomup mushboxen_iter_in_bottomup_init(
   const mushboxen*, const mushbounds*, void* aux);

mushboxen_iter_out mushboxen_iter_out_init(
   const mushboxen*, const mushbounds*, void* aux);

// Iterates over all boxes which overlap with "over" but are also not contained
// in "out".
mushboxen_iter_overout mushboxen_iter_overout_init(
   const mushboxen*, const mushbounds* over, const mushbounds* out, void* aux);

// Common iterator API

#define MUSHBOXEN_ITERATOR_API(I) \
   bool I##_done(I,  const mushboxen*); \
   void I##_next(I*, const mushboxen*);

MUSHBOXEN_ITERATOR_API(mushboxen_iter)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_above)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_below)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_in)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_in_bottomup)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_out)
MUSHBOXEN_ITERATOR_API(mushboxen_iter_overout)

mushaabb* mushboxen_iter_box(mushboxen_iter);
#define MUSHBOXEN_ITER_BOX(i) mushboxen_iter_box(*(const mushboxen_iter*)&(i))
#define mushboxen_iter_above_box(i)       MUSHBOXEN_ITER_BOX(i)
#define mushboxen_iter_below_box(i)       MUSHBOXEN_ITER_BOX(i)
#define mushboxen_iter_in_box(i)          MUSHBOXEN_ITER_BOX(i)
#define mushboxen_iter_in_bottomup_box(i) MUSHBOXEN_ITER_BOX(i)
#define mushboxen_iter_out_box(i)         MUSHBOXEN_ITER_BOX(i)
#define mushboxen_iter_overout_box(i)     MUSHBOXEN_ITER_BOX(i)

// Uncommon iterator API

// Used when the bounds referenced by the iterator were modified. Otherwise
// work like the ordinary _next functions.
void mushboxen_iter_out_updated_next(mushboxen_iter_out*, const mushboxen*);
void mushboxen_iter_overout_updated_next(
   mushboxen_iter_overout*, const mushboxen*);

extern const mushboxen_iter mushboxen_iter_null;
bool mushboxen_iter_is_null(mushboxen_iter);

// Updates the iterator as necessary.
void mushboxen_iter_remove(mushboxen_iter*, mushboxen*);

// Scheduled removal

// Being an iterator-like construct this also needs auxiliary storage.
mushboxen_remsched mushboxen_remsched_init(
   mushboxen*, mushboxen_iter, void* aux);

void mushboxen_iter_in_bottomup_sched_remove(
   mushboxen_iter_in_bottomup*, mushboxen*, mushboxen_remsched*);

void mushboxen_remsched_apply(mushboxen*, mushboxen_remsched*);

#endif
