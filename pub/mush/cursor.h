// File created: 2012-02-06 19:13:57

#ifndef MUSH_CURSOR_H
#define MUSH_CURSOR_H

#include "mush/space.h"

typedef struct mushcursor1  mushcursor1;
typedef struct mushcursor2  mushcursor2;
typedef struct mushcursor3  mushcursor3;
typedef struct mushcursor93 mushcursor93;

extern const size_t mushcursor1_sizeof;
extern const size_t mushcursor2_sizeof;
extern const size_t mushcursor3_sizeof;
extern const size_t mushcursor93_sizeof;

mushcursor1* mushcursor1_init(void*, mushspace1*, mushcoords1);
mushcursor2* mushcursor2_init(void*, mushspace2*, mushcoords2);
mushcursor3* mushcursor3_init(void*, mushspace3*, mushcoords3);
mushcursor93* mushcursor93_init(void*, mushspace93*, mushcoords93);

void mushcursor1_free(mushcursor1*);
void mushcursor2_free(mushcursor2*);
void mushcursor3_free(mushcursor3*);
void mushcursor93_free(mushcursor93*);

mushcursor1* mushcursor1_copy(void*, const mushcursor1*, mushspace1*);
mushcursor2* mushcursor2_copy(void*, const mushcursor2*, mushspace2*);
mushcursor3* mushcursor3_copy(void*, const mushcursor3*, mushspace3*);
mushcursor93* mushcursor93_copy(void*, const mushcursor93*, mushspace93*);

mushcoords1 mushcursor1_get_pos(const mushcursor1*);
mushcoords2 mushcursor2_get_pos(const mushcursor2*);
mushcoords3 mushcursor3_get_pos(const mushcursor3*);
mushcoords93 mushcursor93_get_pos(const mushcursor93*);

void mushcursor1_set_pos(mushcursor1*, mushcoords1);
void mushcursor2_set_pos(mushcursor2*, mushcoords2);
void mushcursor3_set_pos(mushcursor3*, mushcoords3);
void mushcursor93_set_pos(mushcursor93*, mushcoords93);

mushcell mushcursor1_get(mushcursor1*);
mushcell mushcursor2_get(mushcursor2*);
mushcell mushcursor3_get(mushcursor3*);
mushcell93 mushcursor93_get(mushcursor93*);

mushcell mushcursor1_get_unsafe(mushcursor1*);
mushcell mushcursor2_get_unsafe(mushcursor2*);
mushcell mushcursor3_get_unsafe(mushcursor3*);

void mushcursor1_put(mushcursor1*, mushcell);
void mushcursor2_put(mushcursor2*, mushcell);
void mushcursor3_put(mushcursor3*, mushcell);
void mushcursor93_put(mushcursor93*, mushcell93);

void mushcursor1_put_unsafe(mushcursor1*, mushcell);
void mushcursor2_put_unsafe(mushcursor2*, mushcell);
void mushcursor3_put_unsafe(mushcursor3*, mushcell);

void mushcursor1_advance(mushcursor1*, mushcoords1);
void mushcursor2_advance(mushcursor2*, mushcoords2);
void mushcursor3_advance(mushcursor3*, mushcoords3);
void mushcursor93_advance(mushcursor93*, mushcoords93);

void mushcursor1_retreat(mushcursor1*, mushcoords1);
void mushcursor2_retreat(mushcursor2*, mushcoords2);
void mushcursor3_retreat(mushcursor3*, mushcoords3);
void mushcursor93_retreat(mushcursor93*, mushcoords93);

void mushcursor93_wrap(mushcursor93*);

void mushcursor1_skip_markers(mushcursor1*, mushcoords1, mushcell*);
void mushcursor2_skip_markers(mushcursor2*, mushcoords2, mushcell*);
void mushcursor3_skip_markers(mushcursor3*, mushcoords3, mushcell*);
void mushcursor93_skip_markers_98(mushcursor93*, mushcoords93, mushcell*);

void mushcursor1_skip_semicolons(mushcursor1*, mushcoords1, mushcell*);
void mushcursor2_skip_semicolons(mushcursor2*, mushcoords2, mushcell*);
void mushcursor3_skip_semicolons(mushcursor3*, mushcoords3, mushcell*);
void mushcursor93_skip_semicolons(mushcursor93*, mushcoords93, mushcell*);

void mushcursor1_skip_spaces(mushcursor1*, mushcoords1, mushcell*);
void mushcursor2_skip_spaces(mushcursor2*, mushcoords2, mushcell*);
void mushcursor3_skip_spaces(mushcursor3*, mushcoords3, mushcell*);
void mushcursor93_skip_spaces(mushcursor93*, mushcoords93, mushcell*);

void mushcursor1_skip_to_last_space(mushcursor1*, mushcoords1, mushcell*);
void mushcursor2_skip_to_last_space(mushcursor2*, mushcoords2, mushcell*);
void mushcursor3_skip_to_last_space(mushcursor3*, mushcoords3, mushcell*);
void mushcursor93_skip_to_last_space(mushcursor93*, mushcoords93, mushcell*);

#endif
