// File created: 2011-08-06 16:11:09

#ifndef MUSHSPACE_ANAMNESIC_RING_H
#define MUSHSPACE_ANAMNESIC_RING_H

#include <stdbool.h>
#include <stdint.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mush_anamnesic_ring MUSHSPACE_NAME(mush_anamnesic_ring)
#define mush_memory         MUSHSPACE_NAME(mush_memory)

#define MUSH_ANAMNESIC_RING_SIZE 3

typedef struct mush_memory {
	mush_bounds placed;
	mushcoords c;
} mush_memory;

typedef struct mush_anamnesic_ring {
	mush_memory ring[MUSH_ANAMNESIC_RING_SIZE];
	uint8_t pos;
	bool full;
} mush_anamnesic_ring;

#define mush_anamnesic_ring_init MUSHSPACE_CAT(mush_anamnesic_ring,_init)
#define mush_anamnesic_ring_push MUSHSPACE_CAT(mush_anamnesic_ring,_push)
#define mush_anamnesic_ring_read MUSHSPACE_CAT(mush_anamnesic_ring,_read)
#define mush_anamnesic_ring_last MUSHSPACE_CAT(mush_anamnesic_ring,_last)

void mush_anamnesic_ring_init(mush_anamnesic_ring*);

void mush_anamnesic_ring_push(mush_anamnesic_ring*, mush_memory);

void mush_anamnesic_ring_read(const mush_anamnesic_ring*, mush_memory*);
const mush_memory* mush_anamnesic_ring_last(const mush_anamnesic_ring*);

#endif
