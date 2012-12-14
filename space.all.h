// File created: 2011-08-06 15:58:45

#ifndef MUSHSPACE_H
#define MUSHSPACE_H

#include "config/config.h"

#include <stdbool.h>
#include <stdlib.h>

#include "coords.all.h"
#include "errors.any.h"
#include "staticaabb.all.h"
#include "stats.any.h"
#include "typenames.any.h"
#include "config/tunables/overall.h"

#if !MUSHSPACE_93
#include "aabb.98.h"
#include "bakaabb.98.h"
#include "boxen.98.h"
#include "memory.98.h"
#endif

#define mushspace MUSHSPACE_NAME(mushspace)

typedef struct mushspace {
#if MUSHSPACE_93
   mushstaticaabb box;
#else
   mushmemorybuf recent_buf;
   bool just_placed_big;
   mushcoords big_sequence_start, first_placed_big;

   bool (**invalidatees)(void*);
   void  **invalidatees_data;

   mushcoords last_beg, last_end;

   mushstats *stats;
   bool private_stats;

   mushboxen boxen;

#if USE_BAKAABB
   mushbakaabb bak;
#endif

   mushstaticaabb static_box;
#endif

#if MUSH_CAN_SIGNAL
   void (*signal)(musherr, void*, void*);
   void  *signal_data;
#endif
} mushspace;

// What kind of an area is the cursor in? Defined here because
// mushspace_jump_to_box uses it.
typedef enum MushCursorMode {
   MushCursorMode_static,
#if !MUSHSPACE_93
   MushCursorMode_dynamic,
#if USE_BAKAABB
   MushCursorMode_bak,
#endif
#endif
} MushCursorMode;

MUSH_DECL_DYN_ARRAY(mushcell)

#define mushspace_sizeof           MUSHSPACE_CAT(mushspace,_sizeof)
#define mushspace_init             MUSHSPACE_CAT(mushspace,_init)
#define mushspace_free             MUSHSPACE_CAT(mushspace,_free)
#define mushspace_copy             MUSHSPACE_CAT(mushspace,_copy)
#define mushspace_get              MUSHSPACE_CAT(mushspace,_get)
#define mushspace_put              MUSHSPACE_CAT(mushspace,_put)
#define mushspace_get_loose_bounds MUSHSPACE_CAT(mushspace,_get_loose_bounds)
#define mushspace_map_existing     MUSHSPACE_CAT(mushspace,_map_existing)
#define mushspace_map              MUSHSPACE_CAT(mushspace,_map)
#define mushspace_add_invalidatee  MUSHSPACE_CAT(mushspace,_add_invalidatee)
#define mushspace_del_invalidatee  MUSHSPACE_CAT(mushspace,_del_invalidatee)
#define mushspace_invalidate_all   MUSHSPACE_CAT(mushspace,_invalidate_all)
#define mushspace_signal           MUSHSPACE_CAT(mushspace,_signal)
#define mushspace_set_handler      MUSHSPACE_CAT(mushspace,_set_handler)

extern const size_t mushspace_sizeof;

mushspace *mushspace_init(void*
#if !MUSHSPACE_93
                         , mushstats*
#endif
);
void       mushspace_free(mushspace*);
mushspace *mushspace_copy(void*, const mushspace*
#if !MUSHSPACE_93
                         , mushstats*
#endif
);

mushcell mushspace_get(const mushspace*, mushcoords);

void mushspace_put(mushspace*, mushcoords, mushcell);

void mushspace_get_loose_bounds(const mushspace*, mushbounds*);

void mushspace_map_existing(
   mushspace*, mushbounds,
   void(*)(musharr_mushcell, mushcoords, mushcoords, void*),
   void(*)(                  mushcoords, mushcoords, void*), void*);

#if !MUSHSPACE_93
void mushspace_map(
   mushspace*, mushbounds,
   void(*)(musharr_mushcell, mushcoords, mushcoords, void*), void*);

bool mushspace_add_invalidatee(mushspace*, bool(*)(void*), void*);
void mushspace_del_invalidatee(mushspace*, void*);
bool mushspace_invalidate_all (mushspace*);
#endif

#if MUSH_CAN_SIGNAL
_Noreturn void mushspace_signal(const mushspace*, musherr, void*);
#endif

void mushspace_set_handler(mushspace*, void(*)(musherr, void*, void*), void*);

#endif
