#pragma once

#include <stdint.h>
#include <unistd.h>

// - Inspired by a past blog post covering fast userspace concurrency.
// - Scheduling is a bit primitive.
// - TODO: handle error conditions.
// - TODO: full documentation.
// - TODO: ...rewrite in a rubust manner.

#define FIBER __attribute__((noinline))
#define FIBER_LIMIT 400
#define STACK_SIZE 1024
#define NO_READY -1

typedef struct {
    void * context;
    enum {
        Empty,
        Running,
        Ready,
        Sleeping,
    } status;
    struct registers {
        uint64_t rsp;
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
        uint64_t rbp;
    } registers;
    uint64_t stack[STACK_SIZE];
} Fiber;

void fiber_init();
void fiber_exit(void);
void fiber_yield(void);
void fiber_start(void (*)(void *), void *);
void fiber_switch(struct registers *, struct registers *, void *);
void fiber_store(struct registers *);
static void fiber_init_stack(int , void (*)(void *));
static void fiber_wake(int);

Fiber fiber_table[FIBER_LIMIT];
int fiber_id = 0;
int fibers_ready = 1;


int
next_ready(void)
{
    int new_id = (fiber_id + 1) % FIBER_LIMIT;
    for (; new_id != fiber_id; new_id = (new_id + 1) % FIBER_LIMIT) {
        if (fiber_table[new_id].status == Ready) {
            return new_id;
        }
    }

    if (fiber_table[fiber_id].status == Ready) {
        return fiber_id;
    }

    fibers_ready = 0;

    return NO_READY;
}

int
next_empty(void)
{
    if (fiber_table[fiber_id].status == Empty) {
        return fiber_id;
    }

    int new_id = (fiber_id + 1) % FIBER_LIMIT;
    for (; new_id != fiber_id; new_id = (new_id + 1) % FIBER_LIMIT) {
        if (fiber_table[new_id].status == Empty) {
            return new_id;
        }
    }

    return FIBER_LIMIT;
}

void
fiber_init(void)
{
    fiber_store(&fiber_table[0].registers);
    fiber_table[0].status = Running;
}

void
fiber_start(void (*fiber)(void *), void * context)
{
    int new_id = next_empty();

    if (new_id == FIBER_LIMIT) {
        fiber_yield();
        return;
    }

    fiber_init_stack(new_id, fiber);
    fiber_table[new_id].context = context;
    fiber_table[new_id].status = Ready;

    fiber_yield();
}

void
fiber_quit(void)
{
    while (fibers_ready) {
        fiber_yield();
    }

    fiber_switch(&fiber_table[fiber_id].registers, &fiber_table[0].registers, NULL);
}

void
fiber_yield(void)
{
    int new_id = next_ready();

    if (new_id == NO_READY) {
        return;
    }

    if (new_id == fiber_id) {
        fiber_table[fiber_id].status = Running;
        return;
    }

    if (fiber_table[fiber_id].status == Running) {
        fiber_table[fiber_id].status = Ready;
    }

    fiber_wake(new_id);
}

void
fiber_exit(void)
{
    fiber_table[fiber_id].status = Empty;
    fiber_yield();
}

void
fiber_sleep(void)
{
    fiber_table[fiber_id].status = Sleeping;
    fiber_yield();
}

static void
fiber_init_stack(int idx, void (*fiber)(void *))
{
    uint64_t stack_top = STACK_SIZE - 1;
    fiber_table[idx].stack[stack_top] = (uint64_t) fiber_exit;
    fiber_table[idx].stack[stack_top - 1] = (uint64_t) fiber;
    fiber_table[idx].registers.rsp = (uint64_t) &(fiber_table[idx].stack[stack_top - 1]);
}

static void
fiber_wake(int new_id)
{
    if (new_id != fiber_id && fiber_table[new_id].status == Ready) {
       int old_id = fiber_id;
       fiber_id = new_id;

       fiber_table[new_id].status = Running;
       fiber_switch(&fiber_table[old_id].registers,
                    &fiber_table[new_id].registers,
                    fiber_table[new_id].context);
    }
}

// http://www.x86-64.org/documentation/abi.pdf
//void fiber_switch(struct registers * old, struct registers * new, void * context)
__asm__ ("fiber_switch:;"

         "mov     %rsp, 0x00(%rdi);"
         "mov     %r15, 0x08(%rdi);"
         "mov     %r14, 0x10(%rdi);"
         "mov     %r13, 0x18(%rdi);"
         "mov     %r12, 0x20(%rdi);"
         "mov     %rbx, 0x28(%rdi);"
         "mov     %rbp, 0x30(%rdi);"

         "mov     0x00(%rsi), %rsp;"
         "mov     0x08(%rsi), %r15;"
         "mov     0x10(%rsi), %r14;"
         "mov     0x18(%rsi), %r13;"
         "mov     0x20(%rsi), %r12;"
         "mov     0x28(%rsi), %rbx;"
         "mov     0x30(%rsi), %rbp;"

         "mov     %rdx, %rdi;"

         "ret;"
         );

//void fiber_store(struct registers * stored)
__asm__ ("fiber_store:;"

         "mov     %rsp, 0x00(%rdi);"
         "mov     %r15, 0x08(%rdi);"
         "mov     %r14, 0x10(%rdi);"
         "mov     %r13, 0x18(%rdi);"
         "mov     %r12, 0x20(%rdi);"
         "mov     %rbx, 0x28(%rdi);"
         "mov     %rbp, 0x30(%rdi);"

         "ret;"
         );
