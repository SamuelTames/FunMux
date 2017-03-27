CFLAGS = -m64 -ggdb -Wall -std=c11
LDFLAGS = -m64
EXE = fiber_test

$(EXE): main.o fiber.h
	$(CC) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o $(EXE)
