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
		boxes_incorporated;
#else
	char unused;
#endif
} mushstats;

typedef enum {
	MushStat_lookups,
	MushStat_assignments,
	MushStat_boxes_incorporated,
} MushStat;

void mushstats_add(mushstats*, MushStat, uint64_t);

#endif
