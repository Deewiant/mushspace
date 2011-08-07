// File created: 2011-08-06 17:54:41

#include "typenames.h"

#include "coords.h"

typedef struct mush_staticaabb {

#if MUSHSPACE_DIM == 1
		mushcell array[STATICAABB_SIZE_X];
#elif MUSHSPACE_DIM == 2
		mushcell array[STATICAABB_SIZE_X*STATICAABB_SIZE_Y];
#elif MUSHSPACE_DIM == 3
		mushcell array[STATICAABB_SIZE_X*STATICAABB_SIZE_Y*STATICAABB_SIZE_Z];
#endif

} mush_staticaabb;
