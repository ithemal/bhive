#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <assert.h>

#include "harness.h"

uint8_t parse_hex_digit(char c) {
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 0xa;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 0xa;
  return c - '0';
}

uint8_t *hex2bin(char *hex) {
  size_t len = strlen(hex);
  assert(len % 2 == 0);
  uint8_t *bin = (uint8_t *)malloc(len / 2);
  size_t i;
  for (i = 0; i  < len/2; i++) {
    uint8_t hi = parse_hex_digit(hex[i*2]);
    uint8_t lo = parse_hex_digit(hex[i*2 + 1]);
    bin[i] = hi * 16 + lo;
  }
  return bin;
}

int main(int argc, char **argv) {
  char *code_hex = argv[1];
  unsigned int unroll_factor = atoi(argv[2]);
  size_t code_size = strlen(code_hex)/2;
  char *code_to_test = hex2bin(code_hex);

  // allocate 3 pages, the first one for testing
  // the rest for writing down result
  int shm_fd = create_shm_fd("shm-path");

  // `measure` writes the result here
  int l1_read_supported, l1_write_supported, icache_supported;
  struct pmc_counters *counters = measure(
      code_to_test, code_size, unroll_factor,
      &l1_read_supported, &l1_write_supported, &icache_supported, 
      shm_fd);


  if (!counters) {
    fprintf(stderr, "failed to run test\n");
    return 1;
  }

  // print the result, ignore the first set of counters, which is garbage
  printf("Core_cyc\tL1_read_misses\tL1_write_misses\tiCache_misses\tContext_switches\n");
  int i;
  for (i = 1; i < HARNESS_ITERS; i++) {
    printf("%ld\t%ld\t%ld\t%ld\t%ld\n",
        counters[i].core_cyc,
        l1_read_supported ? counters[i].l1_read_misses : -1,
        l1_write_supported ? counters[i].l1_write_misses : -1,
        icache_supported ? counters[i].icache_misses : -1,
        counters[i].context_switches);
  }

  return 0;
}
