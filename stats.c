// File created: 2011-08-09 19:15:00

#include "stats.h"

void mushstats_add(mushstats* stats, MushStat stat, uint64_t val) {
#ifdef ENABLE_STATS
	switch (stat) {
	case MushStat_lookups:            stats->lookups            += val; break;
	case MushStat_assignments:        stats->assignments        += val; break;
	case MushStat_boxes_incorporated: stats->boxes_incorporated += val; break;
	}
#else
	(void)stats; (void)stat; (void)val;
#endif
}
