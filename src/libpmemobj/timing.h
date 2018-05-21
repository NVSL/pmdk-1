/*
 * TODO
 */
#ifndef LIBPMEMOBJ_TIMING_H
#define LIBPMEMOBJ_TIMING_H 1

#include <stdint.h>

enum timing_mode {
    idle = 0,
    durability = 1,
    logging = 2,
    synchronization = 3,
};

void timing_init(void);
void timing_cleanup(void);
void timing_print(void);
void timing_start(uint64_t mode);
void timing_end(uint64_t mode);

#endif
