// File created: 2011-08-07 17:49:34

#include "memory.98.h"

#include <assert.h>
#include <string.h>

void mushmemorybuf_init(mushmemorybuf* ring) {
	ring->pos = 0;
	ring->full = false;
}

void mushmemorybuf_push(mushmemorybuf* ring, mushmemory mem) {
	ring->ring[ring->pos++] = mem;
	if (ring->pos == MUSHMEMORYBUF_SIZE) {
		ring->full = true;
		ring->pos  = 0;
	}
}

void mushmemorybuf_read(const mushmemorybuf* ring, mushmemory* mem) {
	assert (ring->full);

	uint8_t endlen = MUSHMEMORYBUF_SIZE - ring->pos;

	memcpy(mem, ring->ring + ring->pos, endlen * sizeof *mem);
	memcpy(mem + endlen, ring->ring, ring->pos * sizeof *mem);
}

const mushmemory* mushmemorybuf_last(const mushmemorybuf* r) {
	return &r->ring[(r->pos == 0 ? MUSHMEMORYBUF_SIZE : r->pos) - 1];
}
