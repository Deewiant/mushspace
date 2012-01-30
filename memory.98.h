// File created: 2011-08-06 16:11:09

#ifndef MUSHSPACE_MEMORY_H
#define MUSHSPACE_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mushmemorybuf MUSHSPACE_NAME(mushmemorybuf)
#define mushmemory    MUSHSPACE_NAME(mushmemory)

#define MUSHMEMORYBUF_SIZE 3

typedef struct mushmemory {
   mushbounds placed;
   mushcoords c;
} mushmemory;

typedef struct mushmemorybuf {
   mushmemory ring[MUSHMEMORYBUF_SIZE];
   uint8_t pos;
   bool full;
} mushmemorybuf;

#define mushmemorybuf_init MUSHSPACE_CAT(mushmemorybuf,_init)
#define mushmemorybuf_push MUSHSPACE_CAT(mushmemorybuf,_push)
#define mushmemorybuf_read MUSHSPACE_CAT(mushmemorybuf,_read)
#define mushmemorybuf_last MUSHSPACE_CAT(mushmemorybuf,_last)

void mushmemorybuf_init(mushmemorybuf*);

void mushmemorybuf_push(mushmemorybuf*, mushmemory);

void mushmemorybuf_read(const mushmemorybuf*, mushmemory*);
const mushmemory* mushmemorybuf_last(const mushmemorybuf*);

#endif
