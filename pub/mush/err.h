// File created: 2011-08-10 13:40:12

#ifndef MUSH_ERR_H
#define MUSH_ERR_H

enum {
   MUSHERR_NONE                     = 0,
   MUSHERR_OOM                      = 1,
   MUSHERR_NO_ROOM                  = 2,
   MUSHERR_INFINITE_LOOP_SPACES     = 3,
   MUSHERR_INFINITE_LOOP_SEMICOLONS = 4,
   MUSHERR_INVALIDATION_FAILURE     = 5,
   MUSHERR_EMPTY_SPACE              = 6,
};

#endif
