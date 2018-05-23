#include "timing.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define THRESHOLD 1E10 // no operation should take longer
#define CAS(a, b, c) __sync_bool_compare_and_swap(a, b, c)
#define RDTSCP(output) asm volatile( \
    "rdtscp \n" \
    "lfence \n" \
    "shl $32, %%rdx \n" \
    "mov %%eax, %%edx \n" \
    "mov %%rdx, %[time] \n" \
    : \
    : [time] "m" (output) \
    : "rdx", "rax", "rcx")

uint64_t rdtscp(void);

uint64_t rdtscp(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp"
        : "=a" (lo), "=d" (hi)
        :
        : "%rcx");
    asm volatile("lfence" ::: "memory");
    return (uint64_t)lo | (((uint64_t)hi) << 32);
}

typedef struct timing_struct {
    uint32_t current_mode;
    uint32_t total_errors;
    uint64_t t1, t2;
    uint64_t total[4];
    uint64_t next_ptr;
} timing_struct;

uint64_t head = 0;
uint64_t init_cycle = 0;

void timing_init(void) {
    if (init_cycle != 0) return;
    init_cycle = rdtscp();
}

void timing_print(void) {
    uint64_t ptr = head;
    uint64_t total[4] = { 0, 0, 0, 0 };
    uint32_t total_errors = 0;
    while (ptr != 0) {
        for (int i = 0; i < 4; i++) {
            total[i] += ((timing_struct *)ptr)->total[i];
            ((timing_struct *)ptr)->total[i] = 0;
        }
        total_errors += ((timing_struct *)ptr)->total_errors;
        ((timing_struct *)ptr)->total_errors = 0;
        ptr = ((timing_struct *)ptr)->next_ptr;
    }
    FILE *out = stdout;
    fprintf(out, "Timing breakdown (cycles):\n");
    fprintf(out, "Durability,%zu\n", total[0]);
    fprintf(out, "Logging,%zu\n", total[1]);
    fprintf(out, "Synchronization,%zu\n", total[2]);
    fprintf(out, "Allocation,%zu\n", total[3]);
    fprintf(out, "Errors,%d\n", total_errors);
    uint64_t total_cycles = rdtscp() - init_cycle;
    fprintf(out, "Total,%zu\n", total_cycles);
    init_cycle = 0;
    timing_init();
}

void timing_cleanup(void) {
    while (head != 0) {
        uint64_t temp = head;
        head = ((timing_struct *)head)->next_ptr;
        free((void *)temp);
    }
}

__thread timing_struct *my_timing = NULL;

void timing_start(uint32_t mode) {
    // Handling first access
    if (my_timing == NULL) {
        my_timing = (timing_struct *)malloc(sizeof(timing_struct));
        memset(my_timing, 0, sizeof(timing_struct));
        do {
            my_timing->next_ptr = head;
        } while (!CAS(&head, my_timing->next_ptr, (uint64_t)my_timing));
    }

    if (my_timing->current_mode != (uint32_t)idle) return; // not idle
    my_timing->current_mode = mode;
    RDTSCP(my_timing->t1);
}

void timing_end(uint32_t mode) {
    if (my_timing->current_mode != mode) return;
    RDTSCP(my_timing->t2);
    uint64_t latency = my_timing->t2 - my_timing->t1;
    if (latency < THRESHOLD) my_timing->total[mode - 1] += latency;
    else my_timing->total_errors++;
    my_timing->current_mode = (uint32_t)idle; // idle
}
