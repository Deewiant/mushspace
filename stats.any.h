// File created: 2011-08-06 16:12:24

#ifndef MUSHSPACE_STATS_H
#define MUSHSPACE_STATS_H

#include <stdint.h>

#include "config.h"

typedef struct mushstats {
#ifdef MUSH_ENABLE_STATS
	uint64_t
		lookups,
		assignments,
		boxes_incorporated,
		boxes_placed,
		max_boxes_live,
		subsumed_contains,
		subsumed_fusables,
		subsumed_disjoint,
		subsumed_overlaps;
#else
	char unused;
#endif
} mushstats;

typedef enum {
	MushStat_lookups,
	MushStat_assignments,
	MushStat_boxes_incorporated,
	MushStat_boxes_placed,
	MushStat_max_boxes_live,
	MushStat_subsumed_contains,
	MushStat_subsumed_fusables,
	MushStat_subsumed_disjoint,
	MushStat_subsumed_overlaps,
} MushStat;

void mushstats_add(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushstats*, MushStat, uint64_t);

void mushstats_new_max(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushstats*, MushStat, uint64_t);

#endif