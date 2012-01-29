// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

#include <assert.h>

#if !MUSHSPACE_93
#include "space/jump-to-box.98.h"
#endif

#if MUSHSPACE_93
#define STATIC_BOX(sp) (&(sp)->box)
#else
#define STATIC_BOX(sp) (&(sp)->static_box)
#endif

#ifdef MUSH_ENABLE_EXPENSIVE_CURSOR_DEBUGGING
#define DEBUG_CHECK(cursor, c) \
	assert (mushspace_get(cursor->space, mushcursor_get_pos(cursor)) == c)
#else
#define DEBUG_CHECK(cursor, c)
#endif

#if !MUSHSPACE_93
static void mushcursor_recalibrate_void(void*);
#endif

const size_t mushcursor_sizeof = sizeof(mushcursor);

int mushcursor_init(
	mushspace* space, mushcoords pos, mushcoords delta, void** vp)
{
	mushcursor *cursor = *vp ? *vp : (*vp = malloc(sizeof *cursor));
	if (!cursor)
		return MUSHERR_OOM;

#if !MUSHSPACE_93
	if (!mushspace_add_invalidatee(space, mushcursor_recalibrate_void, cursor))
		return MUSHERR_OOM;
#endif

	cursor->space = space;

#if MUSHSPACE_93
	(void)delta;
	cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
#else
	if (!mushcursor_get_box(cursor, pos)) {
		if (!mushspace_jump_to_box(space, &pos, delta, &cursor->mode,
		                           &cursor->box, &cursor->box_idx))
		{
			mushcursor_set_infloop_pos(cursor, pos);
			return MUSHERR_INFINITE_LOOP_SPACES;
		}
		mushcursor_tessellate(cursor, pos);
	}
#endif
	return MUSHERR_NONE;
}

mushcoords mushcursor_get_pos(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mushcoords_add(cursor->rel_pos, MUSHSTATICAABB_BEG);
#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mushcoords_add(cursor->rel_pos, cursor->obeg);
	case MushCursorMode_bak:
		return cursor->actual_pos;
#endif
	}
	assert (false);
}

void mushcursor_set_pos(mushcursor* cursor, mushcoords pos) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
		return;
#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		cursor->rel_pos = mushcoords_sub(pos, cursor->obeg);
		return;
	case MushCursorMode_bak:
		cursor->actual_pos = pos;
		return;
#endif
	}
	assert (false);
}

bool mushcursor_in_box(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mushbounds_contains(&MUSHSTATICAABB_REL_BOUNDS, cursor->rel_pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mushbounds_contains(&cursor->rel_bounds, cursor->rel_pos);

	case MushCursorMode_bak:
		return mushbounds_contains(&cursor->actual_bounds, cursor->actual_pos);
#endif
	}
	assert (false);
}

#if !MUSHSPACE_93
bool mushcursor_get_box(mushcursor* cursor, mushcoords pos) {
	if (mushstaticaabb_contains(pos)) {
		cursor->mode = MushCursorMode_static;
		mushcursor_tessellate(cursor, pos);
		return true;
	}

	mushspace *sp = cursor->space;

	if ((cursor->box = mushspace_find_box_and_idx(sp, pos, &cursor->box_idx))) {
		cursor->mode = MushCursorMode_dynamic;
		mushcursor_tessellate(cursor, pos);
		return true;
	}
	if (sp->bak.data && mushbounds_contains(&sp->bak.bounds, pos)) {
		cursor->mode = MushCursorMode_bak;
		mushcursor_tessellate(cursor, pos);
		return true;
	}
	return false;
}
#endif

mushcell mushcursor_get(mushcursor* cursor) {
#if !MUSHSPACE_93
	if (!mushcursor_in_box(cursor)
	 && !mushcursor_get_box(cursor, mushcursor_get_pos(cursor)))
	{
		DEBUG_CHECK(cursor, ' ');
		return ' ';
	}
#endif
	return mushcursor_get_unsafe(cursor);
}
mushcell mushcursor_get_unsafe(mushcursor* cursor) {
	assert (mushcursor_in_box(cursor));

	mushspace *sp = cursor->space;

	mushcell c;

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		c = mushstaticaabb_get_no_offset(STATIC_BOX(sp), cursor->rel_pos);
		break;

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		c = mushaabb_get_no_offset(cursor->box, cursor->rel_pos);
		break;

	case MushCursorMode_bak:
		c = mushbakaabb_get(&sp->bak, cursor->actual_pos);
		break;
#endif

	default: assert (false);
	}
	DEBUG_CHECK(cursor, c);
	return c;
}

int mushcursor_put(mushcursor* cursor, mushcell c) {
#if !MUSHSPACE_93
	if (!mushcursor_in_box(cursor)) {
		mushcoords pos = mushcursor_get_pos(cursor);
		if (!mushcursor_get_box(cursor, pos)) {
			int ret = mushspace_put(cursor->space, pos, c);
			DEBUG_CHECK(cursor, c);
			return ret;
		}
	}
#endif
	return mushcursor_put_unsafe(cursor, c);
}

int mushcursor_put_unsafe(mushcursor* cursor, mushcell c) {
	assert (mushcursor_in_box(cursor));

	mushspace *sp = cursor->space;

	int ret;

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		mushstaticaabb_put_no_offset(STATIC_BOX(sp), cursor->rel_pos, c);
		ret = MUSHERR_NONE;
		break;

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		mushaabb_put_no_offset(cursor->box, cursor->rel_pos, c);
		ret = MUSHERR_NONE;
		break;

	case MushCursorMode_bak:
		ret = mushbakaabb_put(&sp->bak, cursor->actual_pos, c);
		break;
#endif

	default: assert (false);
	}
	DEBUG_CHECK(cursor, c);
	return ret;
}

void mushcursor_advance(mushcursor* cursor, mushcoords delta) {
#if !MUSHSPACE_93
	if (MUSHCURSOR_MODE(cursor) == MushCursorMode_bak)
		mushcoords_add_into(&cursor->actual_pos, delta);
	else
#endif
		mushcoords_add_into(&cursor->rel_pos, delta);
}

void mushcursor_retreat(mushcursor* cursor, mushcoords delta) {
#if !MUSHSPACE_93
	if (MUSHCURSOR_MODE(cursor) == MushCursorMode_bak)
		mushcoords_sub_into(&cursor->actual_pos, delta);
	else
#endif
		mushcoords_sub_into(&cursor->rel_pos, delta);
}

void mushcursor_recalibrate(mushcursor* cursor) {
#if MUSHSPACE_93
	(void)cursor;
#else
	if (!mushcursor_get_box(cursor, mushcursor_get_pos(cursor))) {
		// Just grab a box which we aren't contained in: get/set can handle it
		// and skip_markers can sort it out. Prefer static because it's the
		// fastest to work with.
		cursor->mode = MushCursorMode_static;
	}
#endif
}
#if !MUSHSPACE_93
static void mushcursor_recalibrate_void(void* cursor) {
	mushcursor_recalibrate(cursor);
}
#endif

#if MUSHSPACE_93
void mushcursor2_93_wrap(mushcursor* cursor) {
	cursor->rel_pos.x %= MUSHSTATICAABB_SIZE.x;
	cursor->rel_pos.y %= MUSHSTATICAABB_SIZE.y;
}
#else
void mushcursor_tessellate(mushcursor* cursor, mushcoords pos) {
	mushspace *sp = cursor->space;

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
		break;

	case MushCursorMode_bak:
		cursor->actual_pos    = pos;
		cursor->actual_bounds = sp->bak.bounds;

		// bak is the lowest, so we tessellate with all boxes.
		mushbounds_tessellate1(&cursor->actual_bounds, pos,
		                       &MUSHSTATICAABB_BOUNDS);
		mushbounds_tessellate (&cursor->actual_bounds, pos,
			(mushcarr_mushbounds){(const mushbounds*)sp->boxen, sp->box_count});
		break;

	case MushCursorMode_dynamic: {
		// cursor->box now becomes only a view. it shares its data array with the
		// original box, but has different bounds. In addition, it is weird: its
		// width and height are not its own, so that index calculation in the
		// _no_offset functions works correctly.
		//
		// BE CAREFUL! Only the *_no_offset functions work properly on it, since
		// the others (notably, _get_idx and thereby _get and _put) tend to
		// depend on the bounds matching the data and the width/height being
		// sensible.

		mushbounds *bounds = &cursor->box->bounds;
		cursor->obeg = bounds->beg;

		// Here we need to tessellate only with the boxes above cursor->box.
		mushbounds_tessellate1(bounds, pos, &MUSHSTATICAABB_BOUNDS);
		mushbounds_tessellate (bounds, pos,
			(mushcarr_mushbounds){(const mushbounds*)sp->boxen, cursor->box_idx});

		cursor->rel_pos    = mushcoords_sub(pos, cursor->obeg);
		cursor->rel_bounds =
			(mushbounds){mushcoords_sub(bounds->beg, cursor->obeg),
			             mushcoords_sub(bounds->end, cursor->obeg)};
		break;
	}

	default: assert (false);
	}
}
#endif

void mushcursor_set_infloop_pos(mushcursor* cursor, mushcoords pos) {
#if !MUSHSPACE_93
	// Since we are "nowhere", we can set an arbitrary mode: any functionality
	// that cares about the mode handles the not-in-a-box case anyway. To save
	// simply the position as-is (no need to mess with relative coordinates),
	// use the bak mode.
	cursor->mode = MushCursorMode_bak;
#endif
	mushcursor_set_pos(cursor, pos);
}
