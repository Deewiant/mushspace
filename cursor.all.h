// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

#if MUSHSPACE_93
#define MUSHCURSOR_MODE(x) MushCursorMode_static
#else
#define MUSHCURSOR_MODE(x) ((x)->mode)
#endif

typedef struct mushcursor {
#if !MUSHSPACE_93
	MushCursorMode mode;
#endif
	mushspace *space;
	union {
		// For dynamic mode.
		struct {
			// For static mode (only rel_pos).
			mushcoords rel_pos;

#if !MUSHSPACE_93
			mush_bounds rel_bounds;
			mushcoords  obeg;
			mush_aabb  *box;
			size_t      box_idx;
#endif
		};
#if !MUSHSPACE_93
		// For bak mode.
		struct {
			mushcoords  actual_pos;
			mush_bounds actual_bounds;
		};
#endif
	};
} mushcursor;

#define mushcursor_sizeof          MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init            MUSHSPACE_CAT(mushcursor,_init)
#define mushcursor_get_pos         MUSHSPACE_CAT(mushcursor,_get_pos)
#define mushcursor_set_pos         MUSHSPACE_CAT(mushcursor,_set_pos)
#define mushcursor_get             MUSHSPACE_CAT(mushcursor,_get)
#define mushcursor_get_unsafe      MUSHSPACE_CAT(mushcursor,_get_unsafe)
#define mushcursor_put             MUSHSPACE_CAT(mushcursor,_put)
#define mushcursor_put_unsafe      MUSHSPACE_CAT(mushcursor,_put_unsafe)
#define mushcursor_advance         MUSHSPACE_CAT(mushcursor,_advance)
#define mushcursor_retreat         MUSHSPACE_CAT(mushcursor,_retreat)
#define mushcursor_skip_spaces     MUSHSPACE_CAT(mushcursor,_skip_spaces)
#define mushcursor_skip_semicolons MUSHSPACE_CAT(mushcursor,_skip_semicolons)
#define mushcursor_recalibrate     MUSHSPACE_CAT(mushcursor,_recalibrate)
#define mushcursor_in_box          MUSHSPACE_CAT(mushcursor,_in_box)
#define mushcursor_get_box         MUSHSPACE_CAT(mushcursor,_get_box)
#define mushcursor_tessellate      MUSHSPACE_CAT(mushcursor,_tessellate)
#define mushcursor_set_infloop_pos MUSHSPACE_CAT(mushcursor,_set_infloop_pos)
#define mushcursor_skip_to_last_space \
	MUSHSPACE_CAT(mushcursor,_skip_to_last_space)

#if MUSHSPACE_93
#define mushcursor_skip_markers mushcursor2_93_skip_markers_98

void mushcursor2_93_wrap(mushcursor*);
#else
#define mushcursor_skip_markers MUSHSPACE_CAT(mushcursor,_skip_markers)
#endif

extern const size_t mushcursor_sizeof;

int mushcursor_init(mushspace*, mushcoords, mushcoords, void**);

mushcoords mushcursor_get_pos(const mushcursor*);
void       mushcursor_set_pos(      mushcursor*, mushcoords);

mushcell mushcursor_get       (mushcursor*);
mushcell mushcursor_get_unsafe(mushcursor*);
int      mushcursor_put       (mushcursor*, mushcell);
int      mushcursor_put_unsafe(mushcursor*, mushcell);

void mushcursor_advance(mushcursor*, mushcoords);
void mushcursor_retreat(mushcursor*, mushcoords);

int mushcursor_skip_markers      (mushcursor*, mushcoords);
int mushcursor_skip_semicolons   (mushcursor*, mushcoords);
int mushcursor_skip_spaces       (mushcursor*, mushcoords);
int mushcursor_skip_to_last_space(mushcursor*, mushcoords);

void mushcursor_recalibrate(mushcursor*);

bool mushcursor_in_box(const mushcursor*);

#if !MUSHSPACE_93
bool mushcursor_get_box(mushcursor*, mushcoords);
#endif

void mushcursor_tessellate(mushcursor*, mushcoords);

void mushcursor_set_infloop_pos(mushcursor*, mushcoords);

#endif
