// File created: 2011-08-10 13:40:12

#ifndef MUSH_ERR_H
#define MUSH_ERR_H

typedef enum {
   MUSHERR_OOM                      = 0,
   MUSHERR_NO_ROOM                  = 1,
   MUSHERR_INFINITE_LOOP_SPACES     = 2,
   MUSHERR_INFINITE_LOOP_SEMICOLONS = 3,
   MUSHERR_INVALIDATION_FAILURE     = 4,
   MUSHERR_EMPTY_SPACE              = 5,
} musherr;

const char* musherr_string(musherr);

#endif
