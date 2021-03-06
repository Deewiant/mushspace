// File created: 2012-01-28 00:49:36

#include "bounds/ray-intersects.98.h"

#include <assert.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

static void check_one_axis(mushcoords, mushcoords, const mushbounds*,
                           mushcell, mushcell, mushdim, bool*, mushucell*);
static bool matches(mushucell, mushcell, mushcell, mushcell, mushcell);
static void get_hittable_range(mushcell, mushcell, mushcell,
                               mushcell*, mushcell*);
static mushucell get_move_count(mushcell, mushcell, mushcell,
                                mushucell*, mushucell*, const mushucell*);
static mushucell get_move_count_expensive(mushcell, mushcell, mushucell*,
                                          mushucell*, const mushucell*);

bool mushbounds_ray_intersects(
   mushcoords o, mushcoords delta, const mushbounds* bounds,
   mushucell* pmove_count, mushcoords* phit_pos)
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
    * 1. Realize that the set of points which can actually be reached with a
    *    given delta is limited to some points near the edge of the box: if the
    *    delta is (1,0), only the leftmost edge of the box can be touched, and
    *    thus only they need to be checked. (See the get_hittable_range()
    *    helper.)
    *
    *    The number of different (coordinate, move count) pairs that we have to
    *    check in this approach is:
    *
    *    sum_i gcd(2^32, delta[i]) * min(|delta[i]|, end[i]-beg[i]+1)
    *
    *    (Where gcd(2^32, delta[i]) is the number of move count solutions for
    *     axis i, assuming a 32-bit mushucell. See mushucell_gcd_lg().)
    *
    * 2. Realize that checking along one axis is sufficient to find all answers
    *    for the whole box. If we go over every solution for every X-coordinate
    *    in the box, there is no point in checking other axes, since any
    *    solutions for them have to have corresponding solutions in the X-axis.
    *
    *    The number of pairs to check here is:
    *
    *    min_i gcd(2^32, delta[i]) * (end[i]-beg[i]+1)
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
    * So let's start with method selection by checking this last inequality.
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

      // See the long explanation above for the meanings of these variables.
      const uint_fast8_t p = mushucell_gcd_lg((mushucell)delta.v[i]);
      const mushucell    g = (mushucell)1 << p;
      const mushucell    s = end->v[i] - beg->v[i] + 1;

      // Note that since we cast to mushucell, this gives the correct result
      // for MUSHCELL_MIN as well.
      const mushucell d = ABS(delta.v[i]);

      if (d >= s) {
         // min(s,d) on this axis equals s. Thus we have min(g*s, ...) on one
         // side of the inequality and g*s + ... on the other. Thus min_pairs_2
         // must be lesser than sum_pairs_1: we're done here.
         goto method2;
      }

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
      const mushucell max_g_multiplicand =
         p == 0 ? MUSHUCELL_MAX
                : (mushucell)1 << (sizeof(mushucell)*8 - p);

      if (s <= max_g_multiplicand) {
         const mushucell gs = g * s;
         if (gs < min_pairs_2) {
            min_pairs_2 = gs;
            axis2 = i;
         }
      } else {
         // g*s overflows, so min_pairs_2 can't possibly grow. Simply ignore
         // that case.
      }

      // As above, but this is for g*d instead of g*s.
      if (d <= max_g_multiplicand) {
         const mushucell gd = g * d;

         // Adding the product to sum_pairs_1 might still overflow.
         if (sum_pairs_1 <= MUSHUCELL_MAX - gd)
            sum_pairs_1 += gd;
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

         check_one_axis(o, delta, bounds, a, b, i,
                        &got_move_count, &best_move_count);
      }
   } else {
method2:
      // Method 2: check all points along a selected axis.

      // If we aborted method selection early, our selected axis might have a
      // zero delta: rectify that.
      if (!delta.v[axis2]) {
         assert (axis2 == 0);
         do ++axis2; while (!delta.v[axis2]);
      }

      check_one_axis(o, delta, bounds, beg->v[axis2], end->v[axis2], axis2,
                     &got_move_count, &best_move_count);
   }
   if (!got_move_count)
      return false;

   *pmove_count = best_move_count;
   *phit_pos    = mushcoords_add(o, mushcoords_muls(delta, best_move_count));
   assert (mushbounds_contains(bounds, *phit_pos));
   return true;
}

static void check_one_axis(
   mushcoords o, mushcoords delta, const mushbounds* bounds,
   mushcell a, mushcell b, mushdim axis,
   bool* got_move_count, mushucell* best_move_count)
{
   const mushcoords *beg = &bounds->beg, *end = &bounds->end;

   // Initialize to avoid undefined behaviour: using an uninitialized value is
   // undefined, even if we do something like multiplying it by zero. After
   // all, our processor might have trap representations for integers.
   mushucell increment = 0;

   // For each coordinate that we might hit (being careful in case b ==
   // MUSHCELL_MAX)...
   for (mushcell p = a; p <= b && p >= a; p = mushcell_inc(p)) {
      // ... figure out the move counts that hit it which would also be
      // improvements to best_move_count.
      mushucell moves;

      const mushucell n = get_move_count(
         o.v[axis], p, delta.v[axis],
         &moves, &increment, *got_move_count ? best_move_count : NULL);

      // For each of the plausible move counts, in order...
      for (mushucell c = 0; c < n; ++c) {
         const mushucell m = moves + c*increment;

         // ... make sure that along the other axes, with the same number of
         // moves, we fall within the box.
         for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) if (i != axis)
            if (!matches(m, beg->v[i], end->v[i], o.v[i], delta.v[i]))
               goto next_move_count;

         // If we did fall within the box, we have a better solution for
         // the whole ray, and we can move to the next point. (Since
         // get_move_count guarantees that any later m's for this point
         // would be greater.)
         *best_move_count = m;
         *got_move_count  = true;
         break;
next_move_count:;
      }
   }
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

   // We have a best_move_count to stay under: reduce count to ensure that we
   // do stay under it.

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
