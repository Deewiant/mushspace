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
mushboxen_iter mushboxen_get_iter(
   const mushboxen* boxen, mushcoords pos, void* aux)
{
   (void)aux;
   return (mushboxen_iter){ mushboxen_get(boxen, pos) };
}

mushboxen_iter mushboxen_insert(mushboxen* boxen, mushaabb* box, void* aux) {
   (void)aux;

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
   mushboxen* boxen, mushboxen_reservation* reserve, mushaabb* box, void* aux)
{
   (void)reserve; (void)aux;
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
   for (size_t i = 0; i < boxen->count; ++i)
      mushbounds_expand_to_cover(bounds, &boxen->ptr[i].bounds);
}

////////////////////////////////////////// Iterator test

// These functions return true if the iterator currently points to an
// acceptable box, with acceptable having a different meaning for each
// iterator.

static bool above_test(mushboxen_iter_above it) {
   return mushbounds_overlaps(
      it.bounds, &mushboxen_iter_above_box(it)->bounds);
}
static bool below_test(mushboxen_iter_below it) {
   return mushbounds_overlaps(
      it.bounds, &mushboxen_iter_below_box(it)->bounds);
}
static bool in_test(mushboxen_iter_in it) {
   return mushbounds_contains_bounds(
      it.bounds, &mushboxen_iter_in_box(it)->bounds);
}
static bool overout_test(mushboxen_iter_overout it) {
   const mushbounds *bounds = &mushboxen_iter_overout_box(it)->bounds;
   return  mushbounds_overlaps       (it.over, bounds)
       && !mushbounds_contains_bounds(it.out,  bounds);
}

////////////////////////////////////////// Iterator init

size_t mushboxen_iter_aux_size(const mushboxen* bn) { (void)bn; return 0; }
const size_t mushboxen_iter_aux_size_init = 0;

mushboxen_iter mushboxen_iter_init(const mushboxen* boxen, void* aux) {
   (void)aux;
   return (mushboxen_iter){ boxen->ptr };
}
mushboxen_iter mushboxen_iter_copy(mushboxen_iter it, void* aux) {
   (void)aux;
   return it;
}
mushboxen_iter_above mushboxen_iter_above_init(
   const mushboxen* boxen, mushboxen_iter sentinel, void* aux)
{
   assert (sentinel.ptr >= boxen->ptr);
   assert (sentinel.ptr <  boxen->ptr + boxen->count);
   mushboxen_iter_above it = {
      .iter     = mushboxen_iter_init(boxen, aux),
      .sentinel = sentinel.ptr,
      .bounds   = &mushboxen_iter_box(sentinel)->bounds,
   };
   if (!mushboxen_iter_above_done(it, boxen) && !above_test(it))
      mushboxen_iter_above_next(&it, boxen);
   return it;
}
mushboxen_iter_below mushboxen_iter_below_init(
   const mushboxen* boxen, mushboxen_iter sentinel, void* aux)
{
   (void)aux;
   (void)boxen;
   assert (sentinel.ptr >= boxen->ptr);
   assert (sentinel.ptr <  boxen->ptr + boxen->count);
   mushboxen_iter_below it = {
      .iter   = (mushboxen_iter){ sentinel.ptr + 1 },
      .bounds = &mushboxen_iter_box(sentinel)->bounds,
   };
   if (!mushboxen_iter_below_done(it, boxen) && !below_test(it))
      mushboxen_iter_below_next(&it, boxen);
   return it;
}
mushboxen_iter_in mushboxen_iter_in_init(
   const mushboxen* boxen, const mushbounds* bounds, void* aux)
{
   mushboxen_iter_in it = (mushboxen_iter_in){
      .iter   = mushboxen_iter_init(boxen, aux),
      .bounds = bounds,
   };
   if (boxen->count && !in_test(it))
      mushboxen_iter_in_next(&it, boxen);
   return it;
}
mushboxen_iter_in_bottomup mushboxen_iter_in_bottomup_init(
   const mushboxen* boxen, const mushbounds* bounds, void* aux)
{
   (void)aux;
   mushboxen_iter_in_bottomup it = (mushboxen_iter_in_bottomup){
      .iter   = (mushboxen_iter){ boxen->ptr + boxen->count - 1 },
      .bounds = bounds,
   };
   if (boxen->count && !in_test(it))
      mushboxen_iter_in_bottomup_next(&it, boxen);
   return it;
}
mushboxen_iter_overout mushboxen_iter_overout_init(
   const mushboxen* boxen, const mushbounds* over, const mushbounds* out,
   void* aux)
{
   mushboxen_iter_overout it = (mushboxen_iter_overout){
      .iter = mushboxen_iter_init(boxen, aux),
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
bool mushboxen_iter_below_done(mushboxen_iter_below it, const mushboxen* boxen)
{
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_in_done(mushboxen_iter_in it, const mushboxen* boxen) {
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_in_bottomup_done(
   mushboxen_iter_in_bottomup it, const mushboxen* boxen)
{
   return it.iter.ptr < boxen->ptr;
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
void mushboxen_iter_above_next(
   mushboxen_iter_above* it, const mushboxen* boxen)
{
   do ++it->iter.ptr;
   while (!mushboxen_iter_above_done(*it, boxen) && !above_test(*it));
}
void mushboxen_iter_below_next(
   mushboxen_iter_below* it, const mushboxen* boxen)
{
   do ++it->iter.ptr;
   while (!mushboxen_iter_below_done(*it, boxen) && !below_test(*it));
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
void mushboxen_iter_overout_next(
   mushboxen_iter_overout* it, const mushboxen* boxen)
{
   do ++it->iter.ptr;
   while (!mushboxen_iter_overout_done(*it, boxen) && !overout_test(*it));
}

////////////////////////////////////////// Iterator misc

void mushboxen_iter_overout_updated_next(
   mushboxen_iter_overout* it, const mushboxen* boxen)
{
   *it = mushboxen_iter_overout_init(boxen, it->over, it->out, NULL);
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

mushboxen_remsched mushboxen_remsched_init(
   mushboxen* boxen, mushboxen_iter it, void* aux)
{
   (void)aux;
   const size_t i = it.ptr - boxen->ptr;
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
