// File created: 2011-08-07 17:49:34

#include "anamnesic_ring.98.h"

#include <assert.h>
#include <string.h>

void mush_anamnesic_ring_init(mush_anamnesic_ring* ring) {
	ring->pos = 0;
	ring->full = false;
}

void mush_anamnesic_ring_push(mush_anamnesic_ring* ring, mush_memory mem) {
	ring->ring[ring->pos++] = mem;
	if (ring->pos == MUSH_ANAMNESIC_RING_SIZE) {
		ring->full = true;
		ring->pos  = 0;
	}
}

void mush_anamnesic_ring_read(
	const mush_anamnesic_ring* ring, mush_memory* mem)
{
	assert (ring->full);

	uint8_t endlen = MUSH_ANAMNESIC_RING_SIZE - ring->pos;

	memcpy(mem, ring->ring + ring->pos, endlen * sizeof *mem);
	memcpy(mem + endlen, ring->ring, ring->pos * sizeof *mem);
}

const mush_memory* mush_anamnesic_ring_last(const mush_anamnesic_ring* r) {
	return &r->ring[(r->pos == 0 ? MUSH_ANAMNESIC_RING_SIZE : r->pos) - 1];
}