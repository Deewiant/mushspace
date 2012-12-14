// File created: 2012-12-14 14:07:09

#include "errors.any.h"

#include <stdio.h>
#include <stdlib.h>

const char* musherr_string(musherr err) {
#define CASE(x) case x: return #x
   switch (err) {
   CASE (MUSHERR_OOM);
   CASE (MUSHERR_NO_ROOM);
   CASE (MUSHERR_INFINITE_LOOP_SPACES);
   CASE (MUSHERR_INFINITE_LOOP_SEMICOLONS);
   CASE (MUSHERR_INVALIDATION_FAILURE);
   CASE (MUSHERR_EMPTY_SPACE);
   }
   return NULL;
}

void musherr_default_handler(musherr err, void* data, void* nothing) {
   (void)data;
   (void)nothing;

   fprintf(stderr, "mushspace :: aborting due to unhandled %s!\n",
           musherr_string(err));
   abort();
}
