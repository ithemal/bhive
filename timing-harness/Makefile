.PHONY: all clean

ifeq ($(NDEBUG), 1)
DEBUG_FLAG = -DNDEBUG=1
else
DEBUG_FLAG =
endif

CC = gcc
CFLAGS = $(DEBUG_FLAG)

all: test

test: test.o run_test.o rdpmc.o libharness.a
	$(CC) -o $@ $^ -lrt

libharness.a: harness.o
	ar -rcs $@ $^

harness.o: harness.c harness.h

rdpmc.o: rdpmc.c

test.o: test.c

run_test.o: run_test.nasm
	nasm -f elf64 -o $@ $< $(DEBUG_FLAG)

clean:
	rm -f run_test.o test.o rdpmc.o harness.o test libharness.a
