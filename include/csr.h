#pragma once

#include <stdint.h>

#define MHARTID  "mhartid"   // hart's unique ID, read-only, set by hardware
#define MTVEC    "mtvec"     // trap handler entry address + mode (direct/vectored)
#define MSTATUS  "mstatus"   // global interrupt enable + privilege mode tracking
#define MEPC     "mepc"      // PC to resume at after a trap is serviced
#define MCAUSE   "mcause"    // reason the last trap occurred (exception/interrupt code)
#define MTVAL    "mtval"     // extra trap info, e.g. faulting address
#define MIE      "mie"       // which interrupt sources are enabled
#define MIP      "mip"       // which interrupts are currently pending
#define MSCRATCH "mscratch"  // scratch storage for the trap handler, no fixed hw meaning

#define CSRR(REG, CSR_REG) \
    asm volatile ("csrr %0, " CSR_REG : "=r" (REG))       // read CSR into REG
#define CSRW(CSR_REG, REG) \
    asm volatile ("csrw " CSR_REG ", %0" : : "r" (REG))   // overwrite entire CSR with REG
#define CSRRW(REG, CSR_REG) \ 
    asm volatile ("csrrw %0, " CSR_REG ", %0" : "+r" (REG) : : "memory") // atomically swap REG and CSR
#define CSRS(CSR_REG, REG) \
    asm volatile ("csrs " CSR_REG ", %0" : : "r" (REG))   // atomically set bits in CSR per REG mask
#define CSRC(CSR_REG, REG) \
    asm volatile ("csrc " CSR_REG ", %0" : : "r" (REG))   // atomically clear bits in CSR per REG mask

