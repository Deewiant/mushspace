// File created: 2011-08-07 17:49:34

#include "anamnesic_ring.98.h"

#include <assert.h>
#include <string.h>

void mushanamnesic_ring_init(mushanamnesic_ring* ring) {
	ring->pos = 0;
	ring->full = false;
}

void mushanamnesic_ring_push(mushanamnesic_ring* ring, mushmemory mem) {
	ring->ring[ring->pos++] = mem;
	if (ring->pos == MUSHANAMNESIC_RING_SIZE) {
		ring->full = true;
		ring->pos  = 0;
	}
}

void mushanamnesic_ring_read(const mushanamnesic_ring* ring, mushmemory* mem) {
	assert (ring->full);

	uint8_t endlen = MUSHANAMNESIC_RING_SIZE - ring->pos;

	memcpy(mem, ring->ring + ring->pos, endlen * sizeof *mem);
	memcpy(mem + endlen, ring->ring, ring->pos * sizeof *mem);
}

const mushmemory* mushanamnesic_ring_last(const mushanamnesic_ring* r) {
	return &r->ring[(r->pos == 0 ? MUSHANAMNESIC_RING_SIZE : r->pos) - 1];
}
