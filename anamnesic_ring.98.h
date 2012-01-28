// File created: 2011-08-06 16:11:09

#ifndef MUSHSPACE_ANAMNESIC_RING_H
#define MUSHSPACE_ANAMNESIC_RING_H

#include <stdbool.h>
#include <stdint.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mushanamnesic_ring MUSHSPACE_NAME(mushanamnesic_ring)
#define mushmemory         MUSHSPACE_NAME(mushmemory)

#define MUSHANAMNESIC_RING_SIZE 3

typedef struct mushmemory {
	mushbounds placed;
	mushcoords c;
} mushmemory;

typedef struct mushanamnesic_ring {
	mushmemory ring[MUSHANAMNESIC_RING_SIZE];
	uint8_t pos;
	bool full;
} mushanamnesic_ring;

#define mushanamnesic_ring_init MUSHSPACE_CAT(mushanamnesic_ring,_init)
#define mushanamnesic_ring_push MUSHSPACE_CAT(mushanamnesic_ring,_push)
#define mushanamnesic_ring_read MUSHSPACE_CAT(mushanamnesic_ring,_read)
#define mushanamnesic_ring_last MUSHSPACE_CAT(mushanamnesic_ring,_last)

void mushanamnesic_ring_init(mushanamnesic_ring*);

void mushanamnesic_ring_push(mushanamnesic_ring*, mushmemory);

void mushanamnesic_ring_read(const mushanamnesic_ring*, mushmemory*);
const mushmemory* mushanamnesic_ring_last(const mushanamnesic_ring*);

#endif
