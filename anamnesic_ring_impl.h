// File created: 2011-08-06 17:54:04

#include <stdbool.h>
#include <stdint.h>

#include "typenames.h"
#include "aabb.h"
#include "coords.h"

#define mush_anamnesic_ring MUSHSPACE_NAME(mush_anamnesic_ring)
#define mush_memory         MUSHSPACE_NAME(mush_memory)

#define MUSH_ANAMNESIC_RING_SIZE 3

typedef struct mush_memory {
	mush_aabb box, placed;
	mushcoords c;
} mush_memory;

typedef struct mush_anamnesic_ring {
	mush_memory ring[MUSH_ANAMNESIC_RING_SIZE];
	uint8_t pos;
	bool full;
} mush_anamnesic_ring;

#define mush_anamnesic_ring_init MUSHSPACE_CAT(mush_anamnesic_ring,_init)

void mush_anamnesic_ring_init(mush_anamnesic_ring*);
