/*****************************************************************************
 *
 * M1522.000800
 * SYSTEM PROGRAMMING
 * 
 * Lab2. Kernel Lab
 *
 * profiler.c
 *  - fork a binary & profile it.
 *
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "chardev.h"

/*****************************************************************************
 * MsrInOut Command Arrays
 */

struct MsrInOut msr_init[] = {
  { MSR_WRITE, PERF_GLOBAL_CTRL, 0x00, 0x00 },  // disable 4 PMCs & 3 FFCs
  { MSR_WRITE, PMC0, 0x00, 0x00 },              // PMC0 initialization
  { MSR_WRITE, PMC1, 0x00, 0x00 },              // PMC1 initialization
  { MSR_WRITE, PMC2, 0x00, 0x00 },              // PMC2 initialization
  { MSR_WRITE, PMC3, 0x00, 0x00 },              // PMC2 initialization
  { MSR_WRITE, FFC0, 0x00, 0x00 },              // PMC2 initialization
  { MSR_WRITE, FFC1, 0x00, 0x00 },              // PMC2 initialization
  { MSR_WRITE, FFC2, 0x00, 0x00 },              // PMC2 initialization
  { MSR_STOP, 0x00, 0x00 }
};

struct MsrInOut msr_set_start[] = { 
  { MSR_WRITE, PERF_EVT_SEL0, INST_RETIRED | PERF_EVT_SEL_USR | PERF_EVT_SEL_OS | PERF_EVT_SEL_TH | PERF_EVT_SEL_EN, 0x00 },
  { MSR_WRITE, PERF_EVT_SEL1, RESOURCE_STALLS | PERF_EVT_SEL_USR | PERF_EVT_SEL_OS | PERF_EVT_SEL_TH | PERF_EVT_SEL_EN, 0x00 },
  { MSR_WRITE, PERF_EVT_SEL2, CPU_CLK_UNHALTED | PERF_EVT_SEL_USR | PERF_EVT_SEL_OS | PERF_EVT_SEL_TH | PERF_EVT_SEL_EN, 0x00 },
  { MSR_WRITE, PERF_GLOBAL_CTRL, PERF_GLOBAL_CTRL_EN_PMC0 | PERF_GLOBAL_CTRL_EN_PMC1 | PERF_GLOBAL_CTRL_EN_PMC2, 0x00},
  { MSR_STOP, 0x00, 0x00 }
};

struct MsrInOut msr_stop_read[] = {
  { MSR_WRITE, PERF_GLOBAL_CTRL, 0x00, 0x00 },  // disable 4 PMCs
  { MSR_WRITE, FIXED_CTR_CTRL, 0x00, 0x00 },    // clean up FFC ctrls
  { MSR_READ, PMC0, 0x00 },
  { MSR_READ, PMC1, 0x00 },
  { MSR_READ, PMC2, 0x00 },
  { MSR_READ, PMC3, 0x00 },
  { MSR_READ, FFC0, 0x00 },
  { MSR_READ, FFC1, 0x00 },
  { MSR_READ, FFC2, 0x00 },
  { MSR_STOP, 0x00, 0x00 }
};

struct MsrInOut msr_rdtsc[] = {
  { MSR_RDTSC, 0x00, 0x00 },
  { MSR_STOP, 0x00, 0x00 }
};

/*****************************************************************************
 * Process tree variable
 */

struct PtreeInfo ptree;

/*****************************************************************************
 * Functions
 */

static int load_device() {
  int fd;
  fd = open("/dev/"DEVICE_NAME, 0);
  if (fd < 0) {
    perror("Failed to open /dev/" DEVICE_NAME);
    exit(1);
  }
  return fd;
}

static void close_device(int fd) {
  int e;
  e = close(fd);
  if (e < 0) {
    perror("Failed to close fd");
    exit(1);
  }
}

void print_header(char *name) {
  printf("-----------------------------------------------------------\n");
  printf("TARGET : %s\n", name);
  printf("\n");
}

int print_ptree(int fd, pid_t pid) {
  printf("- Process Tree Information [pid: %d]\n\n", pid);
  getPtree(fd, pid);
  return 0;
}

void print_profiling(long long nr_inst, long long st_cy, long long cy) {
  double d_nr_inst, d_st_cy, d_cy;

  printf("- Process Mornitoring Information\n\n");
  printf("inst retired :   %20llu\n", nr_inst);
  printf("stalled cycles : %20llu\n", st_cy);
  printf("core cycles :    %20llu\n\n", cy);

  d_nr_inst = (double) nr_inst;
  d_st_cy = (double) st_cy;
  d_cy = (double) cy;

  printf("stall rate :     %20lf %%\n", 100 * d_st_cy/d_cy);
  printf("throughput :     %20lf inst/cycles\n", d_nr_inst/d_cy);
  printf("-----------------------------------------------------------\n");
}

int getPtree(int fd, int pid) {
  struct PtreeInfo pinf;
  int i, stage;
  pinf.current_pid = pid;
  ioctl(fd, IOCTL_GET_PTREE, &pinf);
  if(pinf.current_pid <= 1) {
    stage = 0;
  } else {
    stage = getPtree(fd, pinf.parent_pid);
    for(i = 0; i < stage; i++)
      printf("  ");
    printf("L ");
  }
  printf("%s\n", pinf.current_name);
  return stage+1;
}

int getProfiling(int fd) {
  long long nr_inst = 1, st_cy = 1, cy = 1;
  ioctl(fd, IOCTL_MSR_CMDS, msr_stop_read);
  nr_inst = msr_stop_read[2].value;
  st_cy = msr_stop_read[3].value;
  cy = msr_stop_read[4].value;
  print_profiling(nr_inst, st_cy, cy);
  return 0;
}

int main(int argc, char* argv[]) {
  int pid;
  int fd;

  if (argc <= 1 || argc > 2) {
    printf("Usage : %s {binary}\n", argv[0]);
    return 0;
  }
  
  fd = load_device();
  
  if((pid = fork()) == 0) {
    ioctl(fd, IOCTL_MSR_CMDS, msr_init);
    ioctl(fd, IOCTL_MSR_CMDS, msr_set_start);
    execl(argv[1], argv[1], NULL);
  } else {
    print_header(argv[1]);
    print_ptree(fd, pid);
    wait(0);
    getProfiling(fd);
  }

  close_device(fd);
  return 0;
}
