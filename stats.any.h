// File created: 2011-08-06 16:12:24

#ifndef MUSHSPACE_STATS_H
#define MUSHSPACE_STATS_H

#include <stdint.h>

#include "config.h"

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

typedef enum {
	MushStat_boxes_incorporated,
	MushStat_boxes_placed,
	MushStat_max_boxes_live,
	MushStat_subsumed_contains,
	MushStat_subsumed_fusables,
	MushStat_subsumed_disjoint,
	MushStat_subsumed_overlaps,
	MushStat_empty_boxes_dropped,
} MushStat;

void mushstats_add    (mushstats*, MushStat, uint64_t);
void mushstats_new_max(mushstats*, MushStat, uint64_t);

#endif
