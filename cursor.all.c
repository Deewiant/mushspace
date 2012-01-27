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

#define mushcursor_recalibrate_void MUSHSPACE_CAT(mushcursor,_recalibrate_void)
#define mushcursor_skip_spaces_here MUSHSPACE_CAT(mushcursor,_skip_spaces_here)
#define mushcursor_tessellate       MUSHSPACE_CAT(mushcursor,_tessellate)
#define mushcursor_skip_semicolons_here \
	MUSHSPACE_CAT(mushcursor,_skip_semicolons_here)

static bool mushcursor_skip_spaces_here    (mushcursor*, mushcoords);
static bool mushcursor_skip_semicolons_here(mushcursor*, mushcoords, bool*);
static void mushcursor_set_infloop_pos(mushcursor*, mushcoords);

#ifdef MUSH_ENABLE_INFINITE_LOOP_DETECTION

#define INFLOOP_DECLS \
	mushcoords first_exit; \
	bool is_first_exit = true;

#define INFLOOP_DETECT(pos) do { \
	if (is_first_exit) { \
		is_first_exit = false; \
		first_exit = pos; \
	} else if (mushcoords_equal(pos, first_exit)) { \
		mushcursor_set_infloop_pos(cursor, pos); \
		return MUSH_ERR_INFINITE_LOOP_SPACES; \
	} \
} while (0)

#define INFLOOP_CHECK_DELTA do { \
	if (mushcoords_equal(delta, MUSHCOORDS(0,0,0))) \
		return MUSH_ERR_INFINITE_LOOP_SPACES; \
} while (0)

#else
#define INFLOOP_DECLS
#define INFLOOP_DETECT(pos)
#define INFLOOP_CHECK_DELTA
#endif

#ifdef MUSH_ENABLE_EXPENSIVE_CURSOR_DEBUGGING
#define DEBUG_CHECK(cursor, c) \
	assert (mushspace_get(cursor->space, mushcursor_get_pos(cursor)) == c)
#else
#define DEBUG_CHECK(cursor, c)
#endif

#if MUSHSPACE_93

#define FIND_BOX(cursor, delta, error_code) do { \
	INFLOOP_DETECT(cursor->rel_pos); \
	mushcursor2_93_wrap(cursor); \
} while (0)

#else

#define FIND_BOX(cursor, delta, error_code) do { \
	mushcoords pos = mushcursor_get_pos(cursor); \
	if (mushcursor_get_box(cursor, pos)) \
		continue; \
\
	INFLOOP_DETECT(pos); \
\
	if (!mushspace_jump_to_box(cursor->space, &pos, delta, &cursor->mode, \
	                           &cursor->box, &cursor->box_idx)) \
	{ \
		mushcursor_set_infloop_pos(cursor, pos); \
		return error_code; \
	} \
	mushcursor_tessellate(cursor, pos); \
} while (0)

static void mushcursor_recalibrate_void(void*);
static void mushcursor_tessellate(mushcursor*, mushcoords);
#endif

const size_t mushcursor_sizeof = sizeof(mushcursor);

int mushcursor_init(
	mushspace* space, mushcoords pos, mushcoords delta, void** vp)
{
	mushcursor *cursor = *vp ? *vp : (*vp = malloc(sizeof *cursor));
	if (!cursor)
		return MUSH_ERR_OOM;

#if !MUSHSPACE_93
	mushspace_add_invalidatee(space, mushcursor_recalibrate_void, cursor);
#endif

	cursor->space = space;

#if MUSHSPACE_93
	(void)delta;
	cursor->rel_pos = mushcoords_sub(pos, MUSH_STATICAABB_BEG);
#else
	if (!mushcursor_get_box(cursor, pos)) {
		if (!mushspace_jump_to_box(space, &pos, delta, &cursor->mode,
		                           &cursor->box, &cursor->box_idx))
		{
			mushcursor_set_infloop_pos(cursor, pos);
			return MUSH_ERR_INFINITE_LOOP_SPACES;
		}
		mushcursor_tessellate(cursor, pos);
	}
#endif
	return MUSH_ERR_NONE;
}

mushcoords mushcursor_get_pos(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mushcoords_add(cursor->rel_pos, MUSH_STATICAABB_BEG);
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
		cursor->rel_pos = mushcoords_sub(pos, MUSH_STATICAABB_BEG);
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
		return mush_bounds_contains(&MUSH_STATICAABB_REL_BOUNDS, cursor->rel_pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mush_bounds_contains(&cursor->rel_bounds, cursor->rel_pos);

	case MushCursorMode_bak:
		return mush_bounds_contains(&cursor->actual_bounds, cursor->actual_pos);
#endif
	}
	assert (false);
}

#if !MUSHSPACE_93
bool mushcursor_get_box(mushcursor* cursor, mushcoords pos) {
	if (mush_staticaabb_contains(pos)) {
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
	if (sp->bak.data && mush_bounds_contains(&sp->bak.bounds, pos)) {
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
		c = mush_staticaabb_get_no_offset(STATIC_BOX(sp), cursor->rel_pos);
		break;

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		c = mush_aabb_get_no_offset(cursor->box, cursor->rel_pos);
		break;

	case MushCursorMode_bak:
		c = mush_bakaabb_get(&sp->bak, cursor->actual_pos);
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
		mush_staticaabb_put_no_offset(STATIC_BOX(sp), cursor->rel_pos, c);
		ret = MUSH_ERR_NONE;
		break;

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		mush_aabb_put_no_offset(cursor->box, cursor->rel_pos, c);
		ret = MUSH_ERR_NONE;
		break;

	case MushCursorMode_bak:
		ret = mush_bakaabb_put(&sp->bak, cursor->actual_pos, c);
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

int mushcursor_skip_markers(mushcursor* cursor, mushcoords delta) {
	INFLOOP_DECLS;
	INFLOOP_CHECK_DELTA;

	if (!mushcursor_in_box(cursor))
		goto find_box;

	const mushcell c = mushcursor_get_unsafe(cursor);
	if (c == ';')
		goto semicolon;
	else if (c != ' ')
		return MUSH_ERR_NONE;

	do {
		while (!mushcursor_skip_spaces_here(cursor, delta))
			find_box: FIND_BOX(cursor, delta, MUSH_ERR_INFINITE_LOOP_SPACES);

		if (mushcursor_get_unsafe(cursor) != ';')
			break;
semicolon:;

		bool in_mid = false;
		while (!mushcursor_skip_semicolons_here(cursor, delta, &in_mid))
			FIND_BOX(cursor, delta, MUSH_ERR_INFINITE_LOOP_SEMICOLONS);

	} while (mushcursor_get_unsafe(cursor) == ' ');

	assert (mushcursor_get_unsafe(cursor) != ' ');
	assert (mushcursor_get_unsafe(cursor) != ';');
	return MUSH_ERR_NONE;
}

int mushcursor_skip_spaces(mushcursor* cursor, mushcoords delta) {
	INFLOOP_DECLS;
	INFLOOP_CHECK_DELTA;

	if (!mushcursor_in_box(cursor))
		goto find_box;

	while (!mushcursor_skip_spaces_here(cursor, delta))
		find_box: FIND_BOX(cursor, delta, MUSH_ERR_INFINITE_LOOP_SPACES);

	assert (mushcursor_get(cursor) != ' ');
	return MUSH_ERR_NONE;
}

int mushcursor_skip_to_last_space(mushcursor* cursor, mushcoords delta) {
#if !MUSHSPACE_93
	mushcoords pos;
#endif
	INFLOOP_DECLS;
	INFLOOP_CHECK_DELTA;

	if (!mushcursor_in_box(cursor)) {
#if MUSHSPACE_93
		goto wrap;
#else
		// We should retreat only if we saw at least one space, so don't jump
		// into the loop just because we fell out of the box: that doesn't
		// necessarily mean a space.
		if (!mushcursor_get_box(cursor, pos = mushcursor_get_pos(cursor)))
			goto jump_to_box;
#endif
	}

	if (mushcursor_get_unsafe(cursor) != ' ')
		return MUSH_ERR_NONE;

	while (!mushcursor_skip_spaces_here(cursor, delta)) {
#if MUSHSPACE_93
wrap:
		INFLOOP_DETECT(cursor->rel_pos);
		mushcursor2_93_wrap(cursor);
#else
		if (mushcursor_get_box(cursor, pos = mushcursor_get_pos(cursor)))
			continue;

jump_to_box:
		INFLOOP_DETECT(pos);

		if (!mushspace_jump_to_box(cursor->space, &pos, delta, &cursor->mode,
		                           &cursor->box, &cursor->box_idx))
		{
			mushcursor_set_infloop_pos(cursor, pos);
			return MUSH_ERR_INFINITE_LOOP_SPACES;
		}
		mushcursor_tessellate(cursor, pos);
#endif
	}
	assert (mushcursor_get(cursor) != ' ');
	mushcursor_retreat(cursor, delta);
	assert (mushcursor_get(cursor) == ' ');
	return MUSH_ERR_NONE;
}

static bool mushcursor_skip_spaces_here(mushcursor* cursor, mushcoords delta) {
	assert (mushcursor_in_box(cursor));

	while (mushcursor_get_unsafe(cursor) == ' ') {
		mushcursor_advance(cursor, delta);
		if (!mushcursor_in_box(cursor))
			return false;
	}
	return true;
}

int mushcursor_skip_semicolons(mushcursor* cursor, mushcoords delta) {
	INFLOOP_DECLS;
	INFLOOP_CHECK_DELTA;

	if (!mushcursor_in_box(cursor))
		goto find_box;

	bool in_mid = false;
	while (!mushcursor_skip_semicolons_here(cursor, delta, &in_mid))
		find_box: FIND_BOX(cursor, delta, MUSH_ERR_INFINITE_LOOP_SEMICOLONS);
	assert (mushcursor_get(cursor) != ';');
	return MUSH_ERR_NONE;
}

static bool mushcursor_skip_semicolons_here(
	mushcursor* cursor, mushcoords delta, bool* in_mid)
{
	assert (mushcursor_in_box(cursor));

	if (*in_mid)
		goto continue_prev;

	while (mushcursor_get_unsafe(cursor) == ';') {
		do {
			mushcursor_advance(cursor, delta);
			if (!mushcursor_in_box(cursor)) {
				*in_mid = true;
				return false;
			}
continue_prev:;
		} while (mushcursor_get_unsafe(cursor) != ';');

		mushcursor_advance(cursor, delta);
		if (!mushcursor_in_box(cursor)) {
			*in_mid = false;
			return false;
		}
	}
	return true;
}

#if MUSHSPACE_93
void mushcursor2_93_wrap(mushcursor* cursor) {
	cursor->rel_pos.x %= MUSH_STATICAABB_SIZE.x;
	cursor->rel_pos.y %= MUSH_STATICAABB_SIZE.y;
}
#else
static void mushcursor_tessellate(mushcursor* cursor, mushcoords pos) {
	mushspace *sp = cursor->space;

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		cursor->rel_pos = mushcoords_sub(pos, MUSH_STATICAABB_BEG);
		break;

	case MushCursorMode_bak:
		cursor->actual_pos    = pos;
		cursor->actual_bounds = sp->bak.bounds;

		// bak is the lowest, so we tessellate with all boxes.
		mush_bounds_tessellate1(
			&cursor->actual_bounds, pos, &MUSH_STATICAABB_BOUNDS);
		mush_bounds_tessellate(
			&cursor->actual_bounds, pos,
			(mush_carr_mush_bounds)
				{(const mush_bounds*)sp->boxen, sp->box_count});
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

		mush_bounds *bounds = &cursor->box->bounds;
		cursor->obeg = bounds->beg;

		// Here we need to tessellate only with the boxes above cursor->box.
		mush_bounds_tessellate1(bounds, pos, &MUSH_STATICAABB_BOUNDS);
		mush_bounds_tessellate(bounds, pos,
			(mush_carr_mush_bounds)
				{(const mush_bounds*)sp->boxen, cursor->box_idx});

		cursor->rel_pos    = mushcoords_sub(pos, cursor->obeg);
		cursor->rel_bounds =
			(mush_bounds){mushcoords_sub(bounds->beg, cursor->obeg),
			              mushcoords_sub(bounds->end, cursor->obeg)};
		break;
	}

	default: assert (false);
	}
}
#endif

static void mushcursor_set_infloop_pos(mushcursor* cursor, mushcoords pos) {
#if !MUSHSPACE_93
	// Since we are "nowhere", we can set an arbitrary mode: any functionality
	// that cares about the mode handles the not-in-a-box case anyway. To save
	// simply the position as-is (no need to mess with relative coordinates),
	// use the bak mode.
	cursor->mode = MushCursorMode_bak;
#endif
	mushcursor_set_pos(cursor, pos);
}
