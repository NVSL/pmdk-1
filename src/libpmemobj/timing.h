/*
 * TODO
 */
#ifndef LIBPMEMOBJ_TIMING_H
#define LIBPMEMOBJ_TIMING_H 1

#include <stdint.h>
#include <stdio.h>

enum timing_mode {
    idle = 0,
    durability = 1,
    logging = 2,
    synchronization = 3,
    allocation = 4,
};

void timing_init(void);
void timing_cleanup(void);
void timing_print(void);
void timing_start(uint32_t mode);
void timing_end(uint32_t mode);

void timing_record_start(void);
void timing_record_break(void);
void timing_record_stop(void);
void timing_record_print(FILE *);

#endif
