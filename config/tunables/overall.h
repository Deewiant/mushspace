// File created: 2012-11-20 16:34:18

#ifndef MUSH_CONFIG_TUNABLES_OVERALL_H
#define MUSH_CONFIG_TUNABLES_OVERALL_H

// Which mushboxen implementation to use. Define only one, please!
//#define BOXEN_ARRAY
#define BOXEN_RTREE

// Whether to use mushbakaabb or not. Note that it's useless for -93, since
// that uses only the static box in any case.
//
// Currently only used for arrays, as it doesn't benefit anything else.
#define USE_BAKAABB (!MUSHSPACE_93 && defined(BOXEN_ARRAY))

#endif
