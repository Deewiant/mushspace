// File created: 2012-11-06 13:53:02

typedef struct mushboxen {
   mushaabb *ptr;
   size_t count;
} mushboxen;

typedef struct mushboxen_iter {
   mushaabb *ptr;
} mushboxen_iter, mushboxen_iter_below;

typedef struct {
   mushboxen_iter iter;
   const mushaabb *sentinel;
} mushboxen_iter_above;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *bounds;
} mushboxen_iter_in, mushboxen_iter_in_bottomup, mushboxen_iter_out;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *over, *out;
} mushboxen_iter_overout;

typedef struct mushboxen_reservation { char unused; } mushboxen_reservation;

typedef struct mushboxen_remsched { size_t beg, end; } mushboxen_remsched;
