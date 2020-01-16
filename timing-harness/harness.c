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

#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "rdpmc.h"
#include <linux/perf_event.h>
#include <linux/ioctl.h>

#define SIZE_OF_REL_JUMP (5)

#define SHM_FD 42

#define INIT_VALUE 0x2324000

#define ADDR_OF_AUX_MEM 0x0000700000000000

#define CTX_SWTCH_FD 100

#define OFFSET_TO_COUNTERS 32

#ifndef NDEBUG
#define LOG(args...) printf(args)
#else
#define LOG(...) while(0)
#define perror(prefix) while(0)
#endif

#define SIZE_OF_HARNESS_MEM (4096 * 3)

extern void run_test();
extern void run_test_nop();
// we don't really care about signature of the function here
// just need the address
extern void map_and_restart();
extern void map_aux_and_restart();

// addresses of the `mov rcx, *` instructions
// before we run rdpmc. we modify these instructions
// once we've setup the programmable pmcs
extern char l1_read_misses_a[];
extern char l1_read_misses_b[];
extern char l1_write_misses_a[];
extern char l1_write_misses_b[];
extern char icache_misses_a[];
extern char icache_misses_b[];

// boundary of the actual test harness
extern char code_begin[];
extern char code_end[];

extern char end_tmpl_begin[];
extern char end_tmpl_end[];

extern char first_legal_aux_mem_access[];

struct perf_event_attr l1_read_attr = {
  .type = PERF_TYPE_HW_CACHE,
  .config =
    ((PERF_COUNT_HW_CACHE_L1D) |
     (PERF_COUNT_HW_CACHE_OP_READ << 8) |
     (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)),
  .size = PERF_ATTR_SIZE_VER0,
  .sample_type = PERF_SAMPLE_READ,
  .exclude_kernel = 1
};
struct perf_event_attr l1_write_attr = {
  .type = PERF_TYPE_HW_CACHE,
  .config =
    ((PERF_COUNT_HW_CACHE_L1D) |
     (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
     (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)),
  .size = PERF_ATTR_SIZE_VER0,
  .sample_type = PERF_SAMPLE_READ,
  .exclude_kernel = 1
};
struct perf_event_attr icache_attr = {
  .type = PERF_TYPE_HW_CACHE,
  .config =
    ((PERF_COUNT_HW_CACHE_L1I) |
     (PERF_COUNT_HW_CACHE_OP_READ << 8) |
     (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)),
  .size = PERF_ATTR_SIZE_VER0,
  .sample_type = PERF_SAMPLE_READ,
  .exclude_kernel = 1
};
struct perf_event_attr ctx_swtch_attr = {
  .type = PERF_TYPE_SOFTWARE,
  .config = PERF_COUNT_SW_CONTEXT_SWITCHES,
  .exclude_idle = 0
};

char x;

// round addr to beginning of the page it's in
char *round_to_page_start(char *addr) {
  return (char *)(((uint64_t)addr >> 12) << 12);
}

char *round_to_next_page(char *addr) {
  return (char *)((((uint64_t)addr + 4096) >> 12) << 12);
}

void attach_to_child(pid_t pid, int wr_fd) {
  ptrace(PTRACE_SEIZE, pid, NULL, NULL);
  write(wr_fd, &x, 1);
  close(wr_fd);
}

int create_shm_fd(char *path) {
  int fd = shm_open(path, O_RDWR|O_CREAT, 0777);
  shm_unlink(path);
  ftruncate(fd, SIZE_OF_HARNESS_MEM);
  return fd;
}

int perf_event_open(struct perf_event_attr *hw_event,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags)
{
  return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                 group_fd, flags);
}

void restart_child(pid_t pid, void *restart_addr, void *fault_addr, int shm_fd) {
  LOG("RESTARTING AT %p, fault addr = %p\n", restart_addr, fault_addr);
  struct user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid, NULL, &regs);
  perror("get regs");
  regs.rip = (unsigned long)restart_addr;
  regs.rax = (unsigned long)fault_addr;
  regs.r11 = shm_fd;
  regs.r12 = MAP_SHARED;
  regs.r13 = PROT_READ|PROT_WRITE;
  ptrace(PTRACE_SETREGS, pid, NULL, &regs);
  perror("set regs");
  ptrace(PTRACE_CONT, pid, NULL, NULL);
  perror("cont");
}

// emit inst to do `mov rcx, val'
void emit_mov_rcx(char *inst, int val) {
  if (val < 0)
    return;

  inst[0] = 0xb9;
  inst[1] = val;
  inst[2] = 0;
  inst[3] = 0;
  inst[4] = 0;
}

int is_event_supported(struct perf_event_attr *attr) {
  struct rdpmc_ctx ctx;
  int ok = !rdpmc_open_attr(attr, &ctx, 0);
  rdpmc_close(&ctx);
  return ok;
}

// handling up to these many faults from child process
#define MAX_FAULTS 1024

struct pmc_counters {
  uint64_t core_cyc;
  uint64_t l1_read_misses;
  uint64_t l1_write_misses;
  uint64_t icache_misses;
  uint64_t context_switches;
};

// return counters
struct pmc_counters *measure(
    char *code_to_test,
    unsigned long code_size,
    unsigned int unroll_factor,
    int *l1_read_supported,
    int *l1_write_supported,
    int *icache_supported,
    int shm_fd) {
  int fds[2];
  pipe(fds);

  char *aux_mem = mmap(NULL, 4096 * 2, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 4096);
  perror("mmap aux mem");
  bzero(aux_mem, 4096*2);

  pid_t pid = fork();
  if (pid) { // parent process
    close(fds[0]);

    char *child_mem = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    struct pmc_counters *counters = (struct pmc_counters *)(aux_mem + OFFSET_TO_COUNTERS);

    attach_to_child(pid, fds[1]);

    // find out which PMCs are supported
    *l1_read_supported = is_event_supported(&l1_read_attr);
    *l1_write_supported = is_event_supported(&l1_write_attr);
    *icache_supported = is_event_supported(&icache_attr);

    char *last_failing_inst = 0;

    int mapping_done = 0;

    // TODO: kill the child
    int i;
    for (i = 0; i < MAX_FAULTS; i++) {
      int stat;
      pid = wait(&stat);
      if (WIFEXITED(stat) || pid == -1) {
        int retcode = WEXITSTATUS(stat);
        if (retcode == 0) {
          return counters;
        }
        return NULL;
      }

      // something wrong must have happened
      // find out what happened
      // if we have done mapping then nothing should go wrong
      if (mapping_done) {
        break;
      }
      siginfo_t sinfo;
      ptrace(PTRACE_GETSIGINFO, pid, NULL, &sinfo);
      struct user_regs_struct regs;
      ptrace(PTRACE_GETREGS, pid, NULL, &regs);
      LOG("bad inst is at %p, signal = %d\n", (void *)regs.rip, sinfo.si_signo);
      LOG("VAL OF RSP = %p\n", (void *)regs.rsp);
      if (sinfo.si_signo != 11 && sinfo.si_signo != 5)
        break;

      // find out address causing the segfault
      void *fault_addr = sinfo.si_addr;
      void *restart_addr = &map_and_restart;
      if (sinfo.si_signo == 5)
        fault_addr = (void *)INIT_VALUE;

      if ((char *)regs.rip == last_failing_inst)
        break;


      last_failing_inst = (char *)regs.rip;

      if ((char *)regs.rip == first_legal_aux_mem_access) {
        mapping_done = 1;
        restart_addr = &map_aux_and_restart;
      } else if (fault_addr == (void *)ADDR_OF_AUX_MEM)
        break;

      restart_child(pid, restart_addr, fault_addr, shm_fd);
    }

    // if we've reached this point, the child is hopeless
    kill(pid, SIGKILL);
    return NULL;

  } else { // child process
    // setup PMCs
    struct rdpmc_ctx ctx;
    rdpmc_open_attr(&l1_read_attr, &ctx, 0);
    int l1_read_misses_idx = ctx.buf->index - 1;
    LOG("L1 READ IDX = %d\n", ctx.buf->index);

    rdpmc_open_attr(&l1_write_attr, &ctx, 0);
    int l1_write_misses_idx = ctx.buf->index - 1;
    LOG("L1 WRITE IDX = %d\n", ctx.buf->index);

    rdpmc_open_attr(&icache_attr, &ctx, 0);
    int icache_misses_idx = ctx.buf->index - 1;
    LOG("ICACHE IDX = %d\n", ctx.buf->index);

    int ret = rdpmc_open_attr(&ctx_swtch_attr, &ctx, 0);
    if (ret != 0) {
      LOG("unable to count context switches\n");
      abort();
    }
    dup2(ctx.fd, CTX_SWTCH_FD);

    unsigned long end_tmpl_size = end_tmpl_end - end_tmpl_begin;
    unsigned long total_code_size =
      code_size * unroll_factor + end_tmpl_size + SIZE_OF_REL_JUMP;

    // unprotect the test harness 
    // so that we can emit instructions to use
    // the proper pmc index
    char *begin = round_to_page_start(code_begin);
    char *end = round_to_next_page(code_end + total_code_size);
    errno = 0;
    mprotect(begin, end-begin, PROT_READ|PROT_WRITE);
    perror("mprotect");

    emit_mov_rcx(l1_read_misses_a, l1_read_misses_idx);
    emit_mov_rcx(l1_read_misses_b, l1_read_misses_idx);
    emit_mov_rcx(l1_write_misses_a, l1_write_misses_idx);
    emit_mov_rcx(l1_write_misses_b, l1_write_misses_idx);
    emit_mov_rcx(icache_misses_a, icache_misses_idx);
    emit_mov_rcx(icache_misses_b, icache_misses_idx);

    // copy the *unrolled* user code 
    unsigned char *code_dest = code_end;
    int i;
    for (i = 0; i < unroll_factor; i++) {
      memcpy(code_dest, code_to_test, code_size);
      code_dest += code_size;
    }

    // copy the END measurement code
    memcpy(code_dest, end_tmpl_begin, end_tmpl_size);
    code_dest += end_tmpl_size;

    // insert a rel. jump that brings us back to test_end
    *code_dest = 0xe9;
    *(int *)(code_dest+1) = -(total_code_size + SIZE_OF_REL_JUMP);

    // re-protect the harness
    errno = 0;
    mprotect(begin, end-begin, PROT_EXEC);
    perror("mprotect");

    // wait for parent
    close(fds[1]);
    read(fds[0], &x, 1);
    dup2(shm_fd, SHM_FD);

    // pin this process before we run the test
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(1, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
    setpriority(PRIO_PROCESS, 0, 0);

    // this does not return
    run_test(total_code_size);
  }

  // this is unreachable
  return NULL;
}
