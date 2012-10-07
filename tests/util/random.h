// File created: 2012-10-07 21:27:28

#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

void init_genrand(uint32_t);
extern uint32_t (*genrand_int32)(void);

#endif
