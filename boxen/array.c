// File created: 2012-11-06 13:54:19

#include <assert.h>
#include <string.h>

void mushboxen_init(mushboxen* boxen) {
   boxen->ptr   = NULL;
   boxen->count = 0;
}
void mushboxen_free(mushboxen* boxen) {
   for (size_t i = 0; i < boxen->count; ++i)
      free(boxen->ptr[i].data);
   free(boxen->ptr);
}
bool mushboxen_copy(mushboxen* copy, const mushboxen* boxen) {
   copy->ptr = malloc(boxen->count * sizeof *copy->ptr);
   if (!copy->ptr)
      return false;

   memcpy(copy->ptr, boxen->ptr, copy->count * sizeof *copy->ptr);
   for (size_t i = 0; i < boxen->count; ++i) {
      mushaabb *box = &copy->ptr[i];
      const mushcell *orig = box->data;
      box->data = malloc(box->size * sizeof *box->data);
      if (!box->data) {
         while (i--)
            free(copy->ptr[i].data);
         free(copy->ptr);
         return false;
      }
      memcpy(box->data, orig, box->size * sizeof *box->data);
   }
   copy->count = boxen->count;
   return true;
}

size_t mushboxen_count(const mushboxen* boxen) { return boxen->count; }

mushaabb* mushboxen_get(const mushboxen* boxen, mushcoords pos) {
   for (size_t i = 0; i < boxen->count; ++i)
      if (mushbounds_contains(&boxen->ptr[i].bounds, pos))
         return &boxen->ptr[i];
   return NULL;
}
mushboxen_iter mushboxen_get_iter(const mushboxen* boxen, mushcoords pos) {
   return (mushboxen_iter){ mushboxen_get(boxen, pos) };
}

mushboxen_iter mushboxen_insert(mushboxen* boxen, mushaabb* box) {
   mushaabb *arr = realloc(boxen->ptr, (boxen->count + 1) * sizeof *arr);
   if (!arr)
      return mushboxen_iter_null;
   boxen->ptr = arr;

   mushaabb *ptr = boxen->ptr + boxen->count++;
   *ptr = *box;
   return (mushboxen_iter){ ptr };
}
bool mushboxen_reserve_preserve(
   mushboxen* boxen, mushboxen_reservation* reserve, mushboxen_iter* preserve)
{
   (void)reserve;
   size_t preserve_idx = preserve->ptr - boxen->ptr;

   mushaabb *arr = realloc(boxen->ptr, (boxen->count + 1) * sizeof *arr);
   if (!arr)
      return false;
   boxen->ptr = arr;

   *preserve = (mushboxen_iter){ boxen->ptr + preserve_idx };
   return true;
}
void mushboxen_unreserve(mushboxen* boxen, mushboxen_reservation* reserve) {
   (void)reserve;
   mushaabb *arr = realloc(boxen->ptr, boxen->count * sizeof *arr);
   if (arr)
      boxen->ptr = arr;
}
mushboxen_iter mushboxen_insert_reservation(
   mushboxen* boxen, mushboxen_reservation* reserve, mushaabb* box)
{
   (void)reserve;
   mushboxen_iter it = (mushboxen_iter){ boxen->ptr + boxen->count++ };
   *it.ptr = *box;
   return it;
}

bool mushboxen_contains_bounds(const mushboxen* boxen, const mushbounds* bs) {
   for (size_t i = 0; i < boxen->count; ++i)
      if (mushbounds_contains_bounds(&boxen->ptr[i].bounds, bs))
         return true;
   return false;
}

void mushboxen_loosen_bounds(const mushboxen* boxen, mushbounds* bounds) {
   for (size_t i = 0; i < boxen->count; ++i) {
      mushcoords_min_into(&bounds->beg, boxen->ptr[i].bounds.beg);
      mushcoords_max_into(&bounds->end, boxen->ptr[i].bounds.end);
   }
}

////////////////////////////////////////// Iterator test

// These functions return true if the iterator currently points to an
// acceptable box, with acceptable having a different meaning for each
// iterator.

static bool in_test(mushboxen_iter_in it) {
   return mushbounds_contains_bounds(
      it.bounds, &mushboxen_iter_in_box(it)->bounds);
}
static bool out_test(mushboxen_iter_out it) {
   return !mushbounds_contains_bounds(
      it.bounds, &mushboxen_iter_in_box(it)->bounds);
}
static bool overout_test(mushboxen_iter_overout it) {
   const mushbounds *bounds = &mushboxen_iter_overout_box(it)->bounds;
   return  mushbounds_overlaps       (it.over, bounds)
       && !mushbounds_contains_bounds(it.out,  bounds);
}

////////////////////////////////////////// Iterator init

mushboxen_iter mushboxen_iter_init(const mushboxen* boxen) {
   return (mushboxen_iter){ boxen->ptr };
}
mushboxen_iter_above mushboxen_iter_above_init(
   const mushboxen* boxen, mushboxen_iter sentinel)
{
   assert (sentinel.ptr >= boxen->ptr);
   assert (sentinel.ptr <  boxen->ptr + boxen->count);
   return (mushboxen_iter_above){
      .iter     = mushboxen_iter_init(boxen),
      .sentinel = sentinel.ptr,
   };
}
mushboxen_iter_below mushboxen_iter_below_init(
   const mushboxen* boxen, mushboxen_iter sentinel)
{
   assert (sentinel.ptr >= boxen->ptr);
   assert (sentinel.ptr <  boxen->ptr + boxen->count);
   return (mushboxen_iter_below){
      .iter     = (mushboxen_iter) { sentinel.ptr + 1 },
      .sentinel = boxen->ptr + boxen->count,
   };
}
mushboxen_iter_in mushboxen_iter_in_init(
   const mushboxen* boxen, const mushbounds* bounds)
{
   mushboxen_iter_in it = (mushboxen_iter_in){
      .iter   = mushboxen_iter_init(boxen),
      .bounds = bounds,
   };
   if (boxen->count && !in_test(it))
      mushboxen_iter_in_next(&it, boxen);
   return it;
}
mushboxen_iter_in_bottomup mushboxen_iter_in_bottomup_init(
   const mushboxen* boxen, const mushbounds* bounds)
{
   mushboxen_iter_in_bottomup it = (mushboxen_iter_in_bottomup){
      .iter   = (mushboxen_iter){ boxen->ptr + boxen->count - 1 },
      .bounds = bounds,
   };
   if (boxen->count && !in_test(it))
      mushboxen_iter_in_bottomup_next(&it, boxen);
   return it;
}
mushboxen_iter_out mushboxen_iter_out_init(
   const mushboxen* boxen, const mushbounds* bounds)
{
   mushboxen_iter_out it = (mushboxen_iter_out){
      .iter   = mushboxen_iter_init(boxen),
      .bounds = bounds,
   };
   if (boxen->count && !out_test(it))
      mushboxen_iter_out_next(&it, boxen);
   return it;
}
mushboxen_iter_overout mushboxen_iter_overout_init(
   const mushboxen* boxen, const mushbounds* over, const mushbounds* out)
{
   mushboxen_iter_overout it = (mushboxen_iter_overout){
      .iter = mushboxen_iter_init(boxen),
      .over = over,
      .out  = out,
   };
   if (boxen->count && !overout_test(it))
      mushboxen_iter_overout_next(&it, boxen);
   return it;
}

////////////////////////////////////////// Iterator done

bool mushboxen_iter_done(mushboxen_iter it, const mushboxen* boxen) {
   return it.ptr == boxen->ptr + boxen->count;
}
bool mushboxen_iter_above_done(mushboxen_iter_above it, const mushboxen* b) {
   (void)b;
   return it.iter.ptr == it.sentinel;
}
bool mushboxen_iter_below_done(mushboxen_iter_below it, const mushboxen* b) {
   (void)b;
   return it.iter.ptr == it.sentinel;
}
bool mushboxen_iter_in_done(mushboxen_iter_in it, const mushboxen* boxen) {
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_in_bottomup_done(
   mushboxen_iter_in_bottomup it, const mushboxen* boxen)
{
   return it.iter.ptr < boxen->ptr;
}
bool mushboxen_iter_out_done(mushboxen_iter_out it, const mushboxen* boxen) {
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_overout_done(
   mushboxen_iter_overout it, const mushboxen* boxen)
{
   return mushboxen_iter_done(it.iter, boxen);
}

////////////////////////////////////////// Iterator next

void mushboxen_iter_next(mushboxen_iter* it, const mushboxen* b) {
   (void)b;
   ++it->ptr;
}
void mushboxen_iter_above_next(mushboxen_iter_above* it, const mushboxen* b) {
   mushboxen_iter_next(&it->iter, b);
}
void mushboxen_iter_below_next(mushboxen_iter_below* it, const mushboxen* b) {
   mushboxen_iter_next(&it->iter, b);
}
void mushboxen_iter_in_next(mushboxen_iter_in* it, const mushboxen* boxen) {
   do ++it->iter.ptr;
   while (!mushboxen_iter_in_done(*it, boxen) && !in_test(*it));
}
void mushboxen_iter_in_bottomup_next(
   mushboxen_iter_in_bottomup* it, const mushboxen* boxen)
{
   do --it->iter.ptr;
   while (!mushboxen_iter_in_bottomup_done(*it, boxen) && !in_test(*it));
}
void mushboxen_iter_out_next(mushboxen_iter_out* it, const mushboxen* boxen) {
   do ++it->iter.ptr;
   while (!mushboxen_iter_out_done(*it, boxen) && !out_test(*it));
}
void mushboxen_iter_overout_next(
   mushboxen_iter_overout* it, const mushboxen* boxen)
{
   do ++it->iter.ptr;
   while (!mushboxen_iter_overout_done(*it, boxen) && !overout_test(*it));
}

////////////////////////////////////////// Iterator misc

void mushboxen_iter_out_updated(mushboxen_iter_out* it, const mushboxen* b) {
   (void)it;
   (void)b;
}
void mushboxen_iter_overout_updated(
   mushboxen_iter_overout* it, const mushboxen* b)
{
   (void)it;
   (void)b;
}

mushaabb* mushboxen_iter_box(mushboxen_iter it) { return it.ptr; }

const mushboxen_iter mushboxen_iter_null = { NULL };
bool mushboxen_iter_is_null(mushboxen_iter it) { return it.ptr == NULL; }

void mushboxen_iter_remove(mushboxen_iter* it, mushboxen* boxen) {
   assert (!mushboxen_iter_done   (*it, boxen));
   assert (!mushboxen_iter_is_null(*it));

   mushaabb *p = it->ptr;

   free(p->data);

   memmove(p, p + 1, (--boxen->count - (p - boxen->ptr)) * sizeof *p);
}

mushboxen_remsched mushboxen_remsched_init(mushboxen* bn, mushboxen_iter it) {
   const size_t i = it.ptr - bn->ptr;
   return (mushboxen_remsched){i, i};
}

void mushboxen_iter_in_bottomup_sched_remove(
   mushboxen_iter_in_bottomup* it, mushboxen* boxen, mushboxen_remsched* rs)
{
   size_t i = it->iter.ptr - boxen->ptr;
   if (i == rs->beg - 1) { rs->beg = i; goto end; }
   if (i == rs->end + 1) { rs->end = i; goto end; }

   mushboxen_remsched_apply(boxen, rs);

   if (rs->end < i) {
      const size_t n = rs->end - rs->beg + 1;
      it->iter.ptr -= n;
      i -= n;
   }

   rs->beg = rs->end = i;
end:
   mushboxen_iter_in_bottomup_next(it, boxen);
}

void mushboxen_remsched_apply(mushboxen* boxen, mushboxen_remsched* rs) {
   const size_t b = rs->beg, e = rs->end;

   assert (b <= e);
   assert (e < boxen->count);

   for (size_t i = b; i <= e; ++i)
      free(boxen->ptr[i].data);

   const size_t new_len = boxen->count -= e - b + 1;
   if (b < new_len) {
      mushaabb *arr = boxen->ptr;
      memmove(arr + b, arr + e + 1, (new_len - b) * sizeof *arr);
   }
}
