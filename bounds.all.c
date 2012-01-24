// File created: 2011-08-21 16:17:09

#include "bounds.all.h"

#include <assert.h>

#if !MUSHSPACE_93
// Helpers for mush_bounds_ray_intersects
static bool matches(mushucell, mushcell, mushcell, mushcell, mushcell);
static void get_hittable_range(mushcell, mushcell, mushcell,
                               mushcell*, mushcell*);
static mushucell get_move_count(mushcell, mushcell, mushcell,
                                mushucell*, mushucell*, const mushucell*);
static mushucell get_move_count_expensive(mushcell, mushcell, mushucell*,
                                          mushucell*, const mushucell*);
#endif

size_t mush_bounds_clamped_size(const mush_bounds* bounds) {
	size_t sz = 1;
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		sz = mush_size_t_mul_clamped(
			sz, mush_size_t_add_clamped(bounds->end.v[i] - bounds->beg.v[i], 1));
	return sz;
}

bool mush_bounds_contains(const mush_bounds* bounds, mushcoords pos) {
	if (pos.x < bounds->beg.x || pos.x > bounds->end.x) return false;
#if MUSHSPACE_DIM >= 2
	if (pos.y < bounds->beg.y || pos.y > bounds->end.y) return false;
#if MUSHSPACE_DIM >= 3
	if (pos.z < bounds->beg.z || pos.z > bounds->end.z) return false;
#endif
#endif
	return true;
}
bool mush_bounds_safe_contains(const mush_bounds* bounds, mushcoords pos) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (bounds->beg.v[i] > bounds->end.v[i]) {
			if (!(pos.v[i] >= bounds->beg.v[i] || pos.v[i] <= bounds->end.v[i]))
				return false;
		} else {
			if (!(pos.v[i] >= bounds->beg.v[i] && pos.v[i] <= bounds->end.v[i]))
				return false;
		}
	}
	return true;
}

bool mush_bounds_contains_bounds(const mush_bounds* a, const mush_bounds* b) {
	return mush_bounds_contains(a, b->beg) && mush_bounds_contains(a, b->end);
}
bool mush_bounds_overlaps(const mush_bounds* a, const mush_bounds* b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		if (a->beg.v[i] > b->end.v[i] || b->beg.v[i] > a->end.v[i])
			return false;
	return true;
}

#if !MUSHSPACE_93
bool mush_bounds_get_overlap(
	const mush_bounds* a, const mush_bounds* b, mush_bounds* overlap)
{
	if (!mush_bounds_overlaps(a, b))
		return false;

	overlap->beg = a->beg; mushcoords_max_into(&overlap->beg, b->beg);
	overlap->end = a->end; mushcoords_min_into(&overlap->end, b->end);

	assert (mush_bounds_contains_bounds(a, overlap));
	assert (mush_bounds_contains_bounds(b, overlap));
	return true;
}

#if MUSHSPACE_DIM > 1
bool mush_bounds_on_same_axis(const mush_bounds* a, const mush_bounds* b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
			return true;
	return false;
}
bool mush_bounds_on_same_primary_axis(
	const mush_bounds* a, const mush_bounds* b)
{
	const mushdim I = MUSHSPACE_DIM-1;
	return a->beg.v[I] == b->beg.v[I] && a->end.v[I] == b->end.v[I];
}
#endif

#if !MUSHSPACE_93
bool mush_bounds_can_fuse(const mush_bounds* a, const mush_bounds* b) {
	bool overlap = mush_bounds_overlaps(a, b);

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
			continue;

		if (!(   overlap
		      || mushcell_add_clamped(a->end.v[i], 1) == b->beg.v[i]
		      || mushcell_add_clamped(b->end.v[i], 1) == a->beg.v[i]))
			return false;

		for (mushdim j = i+1; j < MUSHSPACE_DIM; ++j)
			if (a->beg.v[j] != b->beg.v[j] || a->end.v[j] != b->end.v[j])
				return false;

		#if MUSHSPACE_DIM > 1
			assert (mush_bounds_on_same_axis(a, b));
		#endif
		return true;
	}
	return false;
}
#endif

void mush_bounds_tessellate(
	mush_bounds* bounds, mushcoords pos, mush_carr_mush_bounds bs)
{
	assert (mush_bounds_contains(bounds, pos));

	for (size_t i = 0; i < bs.len; ++i)
		if (mush_bounds_overlaps(bounds, &bs.ptr[i]))
			mush_bounds_tessellate1(bounds, pos, &bs.ptr[i]);
}
void mush_bounds_tessellate1(
	mush_bounds* bounds, mushcoords pos, const mush_bounds* avoid)
{
	assert (mush_bounds_contains(bounds, pos));

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		// This could be improved, consider for instance the bottommost box in
		// the following graphic and its current tessellation:
		//
		// +-------+    +--*--*-+
		// |       |    |X .  . |
		// |       |    |  .  . |
		// |     +---   *..*..+---
		// |     |      |  .  |
		// |  +--|      *..+--|
		// |  |  |      |  |  |
		// |  |  |      |  |  |
		// +--|  |      +--|  |
		//
		// (Note that this isn't actually a tessellation: all points will get
		// a rectangle containing the rectangle at X.)
		//
		// Any of the following three would be an improvement (and they would
		// actually be tessellations):
		//
		// +--*--*-+    +-------+    +-----*-+
		// |  .  . |    |       |    |     . |
		// |  .  . |    |       |    |     . |
		// |  .  +---   *.....+---   |     +---
		// |  .  |      |     |      |     |
		// |  +--|      *..+--|      *..+--|
		// |  |  |      |  |  |      |  |  |
		// |  |  |      |  |  |      |  |  |
		// +--|  |      +--|  |      +--|  |

		mushcell ab = avoid->beg.v[i],
		         ae = avoid->end.v[i],
		         p  = pos.v[i];
		if (ae < p) bounds->beg.v[i] = mushcell_max(bounds->beg.v[i], ae+1);
		if (ab > p) bounds->end.v[i] = mushcell_min(bounds->end.v[i], ab-1);
	}
	assert (!mush_bounds_overlaps(bounds, avoid));
}

bool mush_bounds_ray_intersects(
	mushcoords o, mushcoords delta,
	const mush_bounds* bounds, mushucell* pmove_count, mushcoords* phit_pos)
{
	const mushcoords *beg = &bounds->beg, *end = &bounds->end;

	// Quick check to start with: if we don't move along an axis, we should be
	// in the box along it.
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		if (!delta.v[i] && !(o.v[i] >= beg->v[i] && o.v[i] <= end->v[i]))
			return false;

	/* {{{ Long explanation of what happens below
	 *
	 * The basic idea here: check, for each point in the box, how many steps it
	 * takes for the ray to reach it (or whether it can reach it at all). Then
	 * select the minimum as the answer and return true (or, if no points can be
	 * reached, return false).
	 *
	 * What could be done for each point is solving dim-1 linear diophantine
	 * equations, one for each axis. (See the get_move_count() helper.) Thus
	 * we'd get dim-1 sets of move counts that would reach that point. The
	 * minimal solution for the point is then the minimum of their intersection.
	 *
	 * As an optimization, note that we only need to solve one equation, then
	 * simply try each of the resulting move counts for the other axes, checking
	 * whether they also reach the point under consideration.
	 *
	 * This can be extended to reduce the number of points we have to check,
	 * since if we are solving e.g. the equation for the X-coordinate, we
	 * obviously need to do it only for points with a different X-coordinate:
	 * the equation would be the exact same for the others. Now we are no longer
	 * looking at a particular point, rather a line segment within the box. For
	 * the other axes, we now only check that their result falls within the box,
	 * not caring which particular point it hits. (See the matches() helper.)
	 *
	 * We've got two alternative approaches based on the above basic ideas:
	 *
	 * 	1. Realize that the set of points which can actually be reached with a
	 * 	   given delta is limited to some points near the edge of the box: if
	 * 	   the delta is (1,0), only the leftmost edge of the box can be
	 * 	   touched, and thus only they need to be checked. (See the
	 * 	   getBegEnd() helper.)
	 *
	 * 	   The number of different (coordinate, move count) pairs that we have
	 * 	   to check in this approach is:
	 *
	 * 	   sum_i gcd(2^32, delta[i]) * min(|delta[i]|, end[i]-beg[i]+1)
	 *
	 * 	   (Where gcd(2^32, delta[i]) is the number of move count solutions
	 * 	    for axis i, assuming a 32-bit mushucell. See mushucell_gcd_lg().)
	 *
	 * 	2. Realize that checking along one axis is sufficient to find all
	 * 	   answers for the whole box. If we go over every solution for every
	 * 	   X-coordinate in the box, there is no point in checking other axes,
	 * 	   since any solutions for them have to have corresponding solutions
	 * 	   in the X-axis.
	 *
	 * 	   The number of pairs to check here is:
	 *
	 * 	   min_i gcd(2^32, delta[i]) * (end[i]-beg[i]+1)
	 *
	 * To minimize the amount of work we have to do, we want to pick the one
	 * with less pairs to check. So, when does the following inequality hold,
	 * i.e. when do we prefer method 1?
	 *
	 * min(gx*sx, gy*sy, gz*sz) >   gx*min(sx,|dx|)
	 *                            + gy*min(sy,|dy|)
	 *                            + gz*min(sz,|dz|)
	 *
	 * (Where d[xyz] = delta.[xyz], g[xyz] = gcd(2^32, delta.[xyz]), and s[xyz]
	 * = end.[xyz]-beg.[xyz]+1.)
	 *
	 * For this to be true, we want the delta along each axis to be less than
	 * the box size along that axis. Consider, if only two deltas out of three
	 * are less:
	 *
	 * min(gx*sx, gy*sy, gz*sz) > gx*|dx| + gy*|dy| + gz*sz
	 *
	 * One of the summands on the RHS is an argument of the min on the LHS, and
	 * thus the inequality is clearly false since the summands are all positive.
	 *
	 * With all three less, we can't sensibly simplify this any further:
	 *
	 * min(gx*sx, gy*sy, gz*sz) > gx*|dx| + gy*|dy| + gz*|dz|
	 *
	 * So let's start by checking that.
	 * }}} */

	// The number of (coordinate, move count) pairs we need to check in methods
	// 1 and 2 respectively.
	mushucell sum_pairs_1 = 0;
	mushucell min_pairs_2 = MUSHUCELL_MAX;

	// The axis we would check in method 2. We need a default here for the case
	// when everything overflows.
	mushdim axis2 = 0;

	// Not used yet, but defined here so that we don't goto across the
	// initialization: the move count to be written to *pmove_count.
	//
	// The move count can plausibly be MUSHUCELL_MAX so we need an auxiliary
	// boolean to keep track of whether we have a solution.
	mushucell best_move_count;
	bool       got_move_count = false;

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (!delta.v[i])
			continue;

		const uint_fast8_t p = mushucell_gcd_lg((mushucell)delta.v[i]);
		const mushucell    g = (mushucell)1 << p;
		const mushucell    s = end->v[i] - beg->v[i] + 1;

		// Note that since we cast to mushucell, this gives the correct result
		// for MUSHCELL_MIN as well.
		const mushucell d = abs(delta.v[i]);

		// The multiplications can overflow. We can check g*s quickly since we
		// have the gcd_lg:
		//
		//     g * s ≤ MUSHUCELL_MAX
		// <=>     s ≤ MUSHUCELL_MAX / g
		// <=>     s ≤ (2^(sizeof(mushucell)*8) - 1) / 2^p
		// <=>     s ≤ 2^(sizeof(mushucell)*8 - p)
		//
		// But if p is zero, we get 2^(sizeof(mushucell)*8) which is
		// MUSHUCELL_MAX + 1 and therefore overflows to 0. p is zero iff g is
		// one, so just use MUSHUCELL_MAX in that case.
		const mushucell mul_max_s =
			p == 0 ? MUSHUCELL_MAX
			       : (mushucell)1 << (sizeof(mushucell)*8 - p);

		if (s <= mul_max_s) {
			const mushucell gs = g * s;
			if (gs < min_pairs_2) {
				min_pairs_2 = gs;
				axis2 = i;
			}
		} else {
			// g*s overflows, so the minimum doesn't grow, so we can simply ignore
			// that case.
		}

		// For d*s there are no special tricks that I can think of. d ≥ g holds
		// (proof left as an exercise), so I don't think we can do better than
		// the usual division tactic.
		if (s <= MUSHUCELL_MAX / d) {
			const mushucell ds = d * s;

			// Adding the product to sum_pairs_1 might still overflow.
			if (sum_pairs_1 <= MUSHUCELL_MAX - ds)
				sum_pairs_1 += ds;
			else {
				// sum_pairs_1 should now exceed MUSHUCELL_MAX. Either min_pairs_2
				// is lesser, or both values are really huge. In the former case,
				// we know that method 2 is the better choice. In the latter case,
				// we make the reasonable assumption that no matter what we do,
				// it's still going to take a really long time, so just pick a
				// method arbitrarily.
				goto method2;
			}
		} else {
			// As above: sum_pairs_1 should exceed MUSHUCELL_MAX, so just go with
			// method 2.
			goto method2;
		}
	}

	// Now we know which method is cheaper, so use that one and get working. If
	// they're equal, we can pick either. Method 2 seems computationally a bit
	// cheaper in that case (no, I haven't measured it), so do that then.
	if (sum_pairs_1 < min_pairs_2) {
		// Method 1: check the points we could hit along each axis.

		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
			if (!delta.v[i])
				continue;

			// Consider the 1D ray along the axis, and figure out the coordinate
			// range in the box that it can plausibly hit.
			mushcell a, b;
			get_hittable_range(beg->v[i], end->v[i], delta.v[i], &a, &b);

			// If this were the case, we'd have MUSHUCELL_MAX+1 points to check,
			// and thus we'd be using method 2 instead.
			assert (!(a == MUSHCELL_MIN && b == MUSHCELL_MAX));

			// For each coordinate that we might hit (being careful in case b ==
			// MUSHCELL_MAX)...
			for (mushcell p = a; p <= b && p >= a; p = mushcell_inc(p)) {
				// ... figure out the move counts that hit it which would also be
				// improvements to best_move_count.
				mushucell moves, increment;

				const mushucell n = get_move_count(
					o.v[i], p, delta.v[i],
					&moves, &increment, got_move_count ? &best_move_count : NULL);

				// For each of the plausible move counts, in order...
				for (mushucell c = 0; c < n; ++c) {
					mushucell m = moves + c*increment;

					// ... make sure that along the other axes, with the same number
					// of moves, we fall within the box.
					for (mushdim j = 0; j < MUSHSPACE_DIM; ++j) if (i != j)
						if (!matches(m, beg->v[j], end->v[j], o.v[j], delta.v[j]))
							goto next_move_count_1;

					// If we did fall within the box, we have a better solution for
					// the whole ray, and we can move to the next point. (Since
					// get_move_count guarantees that any later m's for this point
					// would be greater.)
					best_move_count = m;
					got_move_count  = true;
					break;
next_move_count_1:;
				}
			}
		}
	} else {
method2:
		// Method 2: check all points along a selected axis. Practically
		// identical to the point-loop in method 1: see the comments there for
		// more info.

		// If we aborted method selection early, our selected axis might have a
		// zero delta: rectify that.
		if (!delta.v[axis2]) {
			assert (axis2 == 0);
			do ++axis2; while (!delta.v[axis2]);
		}

		const mushcell b = beg->v[axis2], e = end->v[axis2];

		for (mushcell p = b; p <= e && p >= b; p = mushcell_inc(p)) {
			mushucell moves, increment;

			const mushucell n = get_move_count(
				o.v[axis2], p, delta.v[axis2],
				&moves, &increment, got_move_count ? &best_move_count : NULL);

			for (mushucell c = 0; c < n; ++c) {
				mushucell m = moves + c*increment;

				for (mushdim j = 0; j < MUSHSPACE_DIM; ++j) if (axis2 != j)
					if (!matches(m, beg->v[j], end->v[j], o.v[j], delta.v[j]))
						goto next_move_count_2;

				best_move_count = m;
				got_move_count  = true;
				break;
next_move_count_2:;
			}
		}
	}
	if (!got_move_count)
		return false;

	*pmove_count = best_move_count;
	*phit_pos    = mushcoords_add(o, mushcoords_muls(delta, best_move_count));
	assert (mush_bounds_contains(bounds, *phit_pos));
	return true;
}
static bool matches(
	mushucell move_count, mushcell a, mushcell b, mushcell from, mushcell delta)
{
	if (!delta) {
		// Zero deltas are checked separately, so we always want true here.
		return true;
	}
	mushcell pos = mushcell_add(from, move_count * (mushucell)delta);
	return pos >= a && pos <= b;
}
static void get_hittable_range(
	mushcell a, mushcell b, mushcell delta, mushcell* hit_a, mushcell* hit_b)
{
	assert (delta != 0);
	if (delta > 0) {
		*hit_a = a;
		*hit_b = mushcell_min(b, mushcell_inc(mushcell_add(a, delta)));
	} else {
		*hit_b = b;
		*hit_a = mushcell_max(a, mushcell_dec(mushcell_add(b, delta)));
	}
}
// The number of moves it takes to get from "from" to "to" with delta "delta".
// Returns the number of such solutions.
//
// Since there may be multiple solutions, gives the minimal solution in
// "moves", the number of solutions as a return value, and the constant
// increment between the solutions in "increment". The value of increment is
// undefined if the count is zero or one.
//
// If given a non-null best, returns a count such that all the resulting move
// counts are lesser than *best.
static mushucell get_move_count(
	mushcell from, mushcell to, mushcell delta,
	mushucell* moves, mushucell* increment, const mushucell* best)
{
	const mushucell diff = (mushucell)to - (mushucell)from;

	mushucell count;

	// Optimize (greatly) for the two typical cases.
	switch (delta) {
	case  1: *moves =  diff; count = (best && *moves >= *best) ? 0 : 1; break;
	case -1: *moves = -diff; count = (best && *moves >= *best) ? 0 : 1; break;
	default:
		count = get_move_count_expensive(diff, delta, moves, increment, best);
		break;
	}
	if (count > 0) {
		for (mushucell i = count; i-- > 1;)
			assert (*moves + (i-1) * *increment < *moves + i * *increment);

		assert (!best || *moves + (count-1) * *increment < *best);
	}
	return count;
}
static mushucell get_move_count_expensive(
	mushcell to, mushcell delta,
	mushucell* move_count, mushucell* increment,
	const mushucell* best_move_count)
{
	mushucell moves;

	uint_fast8_t lg_count;
	if (!mushucell_mod_div((mushucell)delta, to, &moves, &lg_count))
		return 0;

	      mushucell count = (mushucell)1 << lg_count;
	const mushucell incr  = (mushucell)1 << (sizeof(mushucell)*8 - lg_count);

	*increment = incr;

	// Ensure the solutions are in order, with moves being minimal.
	//
	// Since the solutions are cyclical, either they are already in order (i.e.
	// moves is the least and moves + (count-1)*incr is the greatest), or there
	// are two increasing substrings.
	//
	// If the first is lesser than the last, they're in order. (If they're
	// equal, count is 1. A singleton is trivially in order.)
	//
	// E.g. [1 2 3 4 5].
	mushucell last = moves + (count-1)*incr;

	if (moves > last) {
		// Otherwise, we have to find the starting point of the second substring.
		// This binary search does the job.
		//
		// E.g. [3 4 5 1 2] (mod 6).
		mushucell low = 1, high = count;
		for (;;) {
			assert (low < high);

			// Since we start at low = 1 and the number of solutions is always a
			// power of two, this is guaranteed to happen eventually.
			if (high - low == 1) {
				moves += incr * ((moves + low*incr > moves + high*incr)
				                 ? high : low);
				break;
			}

			const mushucell mid = (low + high) >> 1;
			const mushucell val = moves + mid*incr;

			if (val > moves)
				low = mid + 1;
			else {
				assert (val < last);
				high = mid;
			}
		}
	}
	*move_count = moves;

	if (!best_move_count)
		return count;

	// We have a bestMoves to stay under: reduce count to ensure that we do stay
	// under it.

	// Time for another binary search.
	for (mushucell low = 0, high = count;;) {
		assert (low <= high);

		if (high - low <= 1)
			return moves + low*incr >= *best_move_count ? low : high;

		const mushucell mid = (low + high) >> 1;
		const mushucell val = moves + mid*incr;

		if (val < *best_move_count)
			low = mid + 1;
		else
			high = mid;
	}
}
#endif // !MUSHSPACE_93
