// File created: 2011-09-02 19:17:34

#include "space.all.h"

const size_t mushspace_sizeof = sizeof(mushspace);

void mushspace_set_handler(
   mushspace* space, void (*handler)(musherr, void*, void*), void* data)
{
#if MUSH_CAN_SIGNAL
   space->signal      = handler;
   space->signal_data = data;
#else
   (void)space; (void)handler; (void)data;
#endif
}

#if MUSH_CAN_SIGNAL
void mushspace_signal(const mushspace* space, musherr err, void* data) {
   void *signal_data = space->signal_data;
   space->signal          (err, data, signal_data);
   musherr_default_handler(err, data, signal_data);
}
#endif
