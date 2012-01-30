// File created: 2011-08-09 19:15:00

#include "stats.any.h"

#include <assert.h>

void mushstats_add(mushstats* stats, MushStat stat, uint64_t val) {
   switch (stat) {
#define CASE(x) case MushStat_##x: stats->x += val; break;
   CASE(boxes_incorporated)
   CASE(boxes_placed)
   CASE(subsumed_contains)
   CASE(subsumed_fusables)
   CASE(subsumed_disjoint)
   CASE(subsumed_overlaps)
   CASE(empty_boxes_dropped)
#undef CASE
   default: assert(0);
   }
}

void mushstats_new_max(mushstats* stats, MushStat stat, uint64_t val) {
   switch (stat) {
#define CASE(x) case MushStat_##x: if (val > stats->x) stats->x = val; break;
   CASE(max_boxes_live)
#undef CASE
   default: assert(0);
   }
}
