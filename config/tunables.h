// File created: 2012-11-20 16:34:18

#ifndef MUSH_CONFIG_TUNABLES_H
#define MUSH_CONFIG_TUNABLES_H

//#define BOXEN_ARRAY
#define BOXEN_RTREE

#define USE_BAKAABB (!MUSHSPACE_93 && defined(BOXEN_ARRAY))

#if USE_BAKAABB
// Threshold for switching to mushbakaabb. Only limits mushspace_put, not
// mushspace_load_string.
#define MAX_PLACED_BOXEN 64
#endif

// Padding of box created by default when mushspace_putting to an unallocated
// area. The size of the resulting box will be 2*NEWBOX_PAD+1 along each axis.
// (Clamped to the edges of space to avoid allocating more than one box.)
#define NEWBOX_PAD 8

// One-directional padding of box created when heuristics determine that
// something like an array is being written by mushspace_put.
#define BIGBOX_PAD 512

// How much leeway we have in heuristic big box requirement testing. If you're
// mushspace_putting a one-dimensional area, this is the maximum stride you can
// use for it to be noticed, causing a big box to be placed.
//
// Implicitly defines an ACCEPTABLE_WASTE for BIGBOXes: it's
// (BIG_SEQ_MAX_SPACING - 1) * BIGBOX_PAD^(MUSHSPACE_DIM-1).
//
// This is a distance between two cells, not the number of spaces between them,
// and thus should always be at least 1.
#define BIG_SEQ_MAX_SPACING 4

// How much space we can live with allocating "uselessly" when joining AABBs
// together. For example, in Unefunge, this is the maximum distance allowed
// between two boxes.
//
// A box 5 units wide, otherwise of size NEWBOX_PAD.
#define ACCEPTABLE_WASTE_Y (MUSHSPACE_DIM >= 2 ? NEWBOX_PAD : 1)
#define ACCEPTABLE_WASTE_Z (MUSHSPACE_DIM >= 3 ? NEWBOX_PAD : 1)
#define ACCEPTABLE_WASTE   (5 * ACCEPTABLE_WASTE_Y * ACCEPTABLE_WASTE_Z)

#endif
