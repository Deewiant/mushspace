// File created: 2012-02-06 19:13:57

#ifndef MUSH_STATS_H
#define MUSH_STATS_H

#include <stdint.h>

typedef struct mushstats {
   uint64_t
      boxes_incorporated,
      boxes_placed,
      max_boxes_live,
      subsumed_contains,
      subsumed_fusables,
      subsumed_disjoint,
      subsumed_overlaps,
      empty_boxes_dropped;
} mushstats;

#endif
