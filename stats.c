// File created: 2011-08-09 19:15:00

#include "stats.h"

#include <assert.h>

void mushstats_add(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushstats* stats, MushStat stat, uint64_t val) {
#ifdef MUSH_ENABLE_STATS
	switch (stat) {
#define CASE(x) case MushStat_##x: stats->x += val; break;
	CASE(lookups)
	CASE(assignments)
	CASE(boxes_incorporated)
	CASE(boxes_placed)
	CASE(subsumed_contains)
	CASE(subsumed_fusables)
	CASE(subsumed_disjoint)
	CASE(subsumed_overlaps)
#undef CASE
	}
#else
	(void)stats; (void)stat; (void)val;
#endif
}

void mushstats_new_max(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushstats* stats, MushStat stat, uint64_t val) {
#ifdef MUSH_ENABLE_STATS
	switch (stat) {
#define CASE(x) case MushStat_##x: if (val > stats->x) stats->x = val; break;
#undef CASE
	default: assert(0);
	}
#else
	(void)stats; (void)stat; (void)val;
#endif
}
