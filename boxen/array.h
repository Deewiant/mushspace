// File created: 2012-11-06 13:53:02

// An array.
//
// T-ordering is implemented trivially: for any box A that is T-above another
// box B, the index of A is less than the index of B.
typedef struct mushboxen {
   mushaabb *ptr;
   size_t count, capacity;
} mushboxen;

typedef struct mushboxen_iter {
   mushaabb *ptr;
} mushboxen_iter;

typedef struct {
   mushboxen_iter iter;
   const mushaabb *sentinel;
   const mushbounds *bounds;
} mushboxen_iter_above;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *bounds;
} mushboxen_iter_below,
  mushboxen_iter_in, mushboxen_iter_in_bottomup;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *over, *out;
} mushboxen_iter_overout;

typedef struct mushboxen_reservation { char unused; } mushboxen_reservation;

typedef struct mushboxen_remsched { size_t beg, end; } mushboxen_remsched;
