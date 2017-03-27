#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "fiber.h"

FIBER
void
f(void * context)
{
    int id = (int)context;
    char c;
    int me = open("Makefile", O_RDONLY);

    for (int i = 0; i < 6; i++) {
        read(me, &c, sizeof c);
        write(STDIN_FILENO, &"             "
                             "             ", id*2);
        printf("Fiber:%d - %c\n", id, c);
        fiber_yield();
    }

    close(me);
}

int
main(void)
{
    printf("Fibers starting!\n");

    fiber_init();
    for (int i = 1; i <= 10; i += 1) {
        void * context = (void *) i;
        fiber_start(f, context);
    }
    fiber_quit();

    printf("Fibers done!\n");
}
