#include "timing.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CAS(a, b, c) __sync_bool_compare_and_swap(a, b, c)
#define RDTSC(output) asm volatile( \
    "rdtscp \n" \
    "mfence \n" \
    "shl $32, %%rdx \n" \
    "mov %%eax, %%edx \n" \
    "mov %%rdx, %[time] \n" \
    : \
    : [time] "m" (output) \
    : "rdx", "rax", "rcx")

typedef struct timing_struct {
    uint64_t current_mode;
    uint64_t t1, t2;
    uint64_t total[4];
    uint64_t next_ptr;
} timing_struct;

uint64_t head = 0;
uint64_t init_cycle = 0;

void timing_init(void) {
    RDTSC(init_cycle);
}

void timing_print(void) {
    uint64_t ptr = head;
    uint64_t total[4] = { 0, 0, 0, 0 };
    while (ptr != 0) {
        for (int i = 0; i < 4; i++) {
            total[i] += ((timing_struct *)ptr)->total[i];
            ((timing_struct *)ptr)->total[i] = 0;
        }
        ptr = ((timing_struct *)ptr)->next_ptr;
    }
    fprintf(stdout, "Timing breakdown (cycles):\n");
    fprintf(stdout, "Durability:\t\t%zu\n", total[0]);
    fprintf(stdout, "Logging:\t\t%zu\n", total[1]);
    fprintf(stdout, "Synchronization:\t%zu\n", total[2]);
    fprintf(stdout, "Allocation:\t\t%zu\n", total[3]);
    uint64_t total_cycles;
    RDTSC(total_cycles);
    total_cycles -= init_cycle;
    fprintf(stdout, "Total:\t\t\t%zu\n", total_cycles);
}

void timing_cleanup(void) {
    while (head != 0) {
        uint64_t temp = head;
        head = ((timing_struct *)head)->next_ptr;
        free((void *)temp);
    }
}

__thread timing_struct *my_timing = NULL;

void timing_start(uint64_t mode) {
    // Handling first access
    if (my_timing == NULL) {
        my_timing = (timing_struct *)malloc(sizeof(timing_struct));
        memset(my_timing, 0, sizeof(timing_struct));
        do {
            my_timing->next_ptr = head;
        } while (!CAS(&head, my_timing->next_ptr, (uint64_t)my_timing));
    }

    if (my_timing->current_mode != (uint64_t)idle) return; // not idle
    my_timing->current_mode = mode;
    RDTSC(my_timing->t1);
}

void timing_end(uint64_t mode) {
    if (my_timing->current_mode != mode) return;
    RDTSC(my_timing->t2);
    my_timing->total[mode - 1] += (my_timing->t2 - my_timing->t1);
    my_timing->current_mode = (uint64_t)idle; // idle
}
