#ifndef LIBPMEMOBJ_TIMING_H
#define LIBPMEMOBJ_TIMING_H 1

#define CAS(a, b, c) __sync_bool_compare_and_swap(a, b, c)

enum timing_mode {
    idle = 0,
    durability = 1,
    logging = 2,
    synchronization = 3,
};

typedef struct timing_struct {
    uint64_t current_mode;
    uint64_t t1, t2;
    uint64_t total[3];
    uint64_t reserved;
    uint64_t next_ptr;
} timing_struct;

void timing_print(void);
void timing_start(enum timing_mode mode);
void timing_end(enum timing_mode mode);

static uint64_t head = 0;

void timing_print(void) {
    timing_struct *ptr = (timing_struct *)head;
    uint64_t total[3] = { 0, 0, 0 };
    while (ptr != NULL) {
        for (int i = 0; i < 3; i++) {
            total[i] += ptr->total[i];
        }
        timing_struct *temp = ptr;
        ptr = (timing_struct *)ptr->next_ptr;
        free(temp);
    }
    fprintf(stdout, "Timing breakdown (cycles):\n");
    fprintf(stdout, "Durability:\t\t%zu\n", total[0]);
    fprintf(stdout, "Logging:\t\t%zu\n", total[1]);
    fprintf(stdout, "Synchronization:\t%zu\n", total[2]);
}

__thread timing_struct *my_timing = NULL;

void timing_start(enum timing_mode mode) {
    if (my_timing == NULL) {
        my_timing = (timing_struct *)malloc(sizeof(timing_struct));
        memset(my_timing, 0, sizeof(timing_struct));
        do {
            my_timing->next_ptr = head;
        } while (!CAS(&head, my_timing->next_ptr, (uint64_t)my_timing));
    }

    if (my_timing->current_mode != idle) return;
    my_timing->current_mode = mode;
    asm volatile(
        "rdtscp \n"
        "mfence \n"
        "shl $32, %%rdx \n"
        "mov %%eax, %%edx \n"
        "mov %%rdx, %[time] \n"
    :
    : [time] "m" (my_timing->t1)
    : "rdx", "rax");
}

void timing_end(enum timing_mode mode) {
    if (my_timing->current_mode != mode) return;
    asm volatile(
        "mfence \n"
        "rdtscp \n"
        "shl $32, %%rdx \n"
        "mov %%eax, %%edx \n"
        "mov %%rdx, %[time] \n"
    :
    : [time] "m" (my_timing->t2)
    : "rdx", "rax");
    my_timing->total[mode - 1] += my_timing->t2 - my_timing->t1;
    my_timing->current_mode = idle;
}

#endif
