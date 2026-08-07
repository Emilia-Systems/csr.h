# csr.h

A single-header, dependency-free C interface for reading and writing RISC-V
Control and Status Registers (CSRs) from freestanding C code, bootloaders,
kernels, and other bare-metal targets where you don't have (and don't want)
a libc.

No build system, no linking step: `#include "csr.h"` and go.

## Why this exists

RISC-V CSRs (`mtvec`, `mstatus`, `mhartid`, etc.) live in a register space
completely separate from the 32 general-purpose registers. They can only be
read or written by a dedicated family of instructions (`csrr`, `csrw`,
`csrs`, `csrc`, ...), there is no way to touch them with ordinary C code,
and the CSR name has to be known at **compile time**, since it's encoded
directly into the instruction itself rather than passed as a runtime value.

That means a normal C function can't take "which CSR" as a parameter — the
only way to keep this from becoming one hand-written inline-asm block per
register is to generate that inline asm from a macro, with the CSR name
baked in at the call site. `csr.h` is that macro layer: four operations,
a name for every CSR you're likely to touch early in a boot/kernel
codebase, and nothing else.

## What's in the header

### Register names

Each `#define` maps a short, typo-resistant name to the CSR's actual
assembly-level string, so a mistyped macro name fails loudly at
**compile time** (undeclared identifier) rather than surfacing later as an
opaque assembler error buried inside generated instruction text.

| Macro | CSR | What it holds |
|---|---|---|
| `MHARTID` | `mhartid` | This hart's unique ID. Read-only, assigned by hardware. |
| `MTVEC` | `mtvec` | Trap handler entry address, plus direct/vectored mode bit. |
| `MSTATUS` | `mstatus` | Global interrupt enable and current privilege-mode tracking. |
| `MEPC` | `mepc` | The PC to resume at once the current trap has been serviced. |
| `MCAUSE` | `mcause` | Why the last trap fired (which exception or interrupt code). |
| `MTVAL` | `mtval` | Supplementary trap info — e.g. the faulting address. |
| `MIE` | `mie` | Which interrupt sources are currently enabled. |
| `MIP` | `mip` | Which interrupts are currently pending. |
| `MSCRATCH` | `mscratch` | General-purpose scratch storage for the trap handler; no fixed hardware meaning. |

This list covers what a bootloader and an early, minimal trap handler need.
Supervisor-mode CSRs (`satp`, `stvec`, `sstatus`, ...) are intentionally
left out, I will eventually add them when the kernel actually starts using virtual memory
or S-mode, not before.

### Operations

All four follow RISC-V's own instruction operand order (destination first,
source second), so a macro call reads exactly like the instruction it
generates.

| Macro | Instruction | Effect |
|---|---|---|
| `CSRR(reg, CSR)` | `csrr` | Read `CSR` into the C variable `reg`. |
| `CSRW(CSR, reg)` | `csrw` | Overwrite all of `CSR` with the value in `reg`. |
| `CSRS(CSR, reg)` | `csrs` | Atomically **set** the bits in `CSR` marked `1` in `reg`; other bits untouched. |
| `CSRC(CSR, reg)` | `csrc` | Atomically **clear** the bits in `CSR` marked `1` in `reg`; other bits untouched. |

`CSRS`/`CSRC` matter for registers like `mstatus`, where many unrelated
fields are packed into one word, a plain `CSRW` would require a
read-modify-write done manually in C, with a window where a trap could
land between the read and the write and corrupt the update. `csrs`/`csrc`
do that read-modify-write as a single atomic hardware instruction.

## Usage

```c
#include "csr.h"

uintptr_t hartid;
CSRR(hartid, MHARTID);        // hartid now holds this core's ID

CSRW(MTVEC, trap_handler_addr);   // point mtvec at your trap handler

uintptr_t enable_bit = (1UL << 3); // e.g. machine interrupt enable
CSRS(MSTATUS, enable_bit);    // set just that bit, leave the rest alone
CSRC(MSTATUS, enable_bit);    // clear it again, leave the rest alone
```

## How it works

Each macro expands to a single [extended GCC/Clang inline-asm]
statement. The CSR name macro (e.g. `MTVEC`) expands to a plain string
literal, which the C preprocessor concatenates directly into the asm
template text (`"csrr %0, " MTVEC` -> `"csrr %0, mtvec"`). The register
argument (`reg`) is passed as a genuine output or input **operand**
(`"=r"(reg)` / `"r"(reg)`), not stringified, this is what lets the
compiler pick the actual physical register and read or write your C
variable directly, rather than the asm block being an opaque black box
the compiler can't reason about.

All four macros are marked `volatile`, as prevention, so the compiler will not reorder,
cache, or eliminate the instruction, required for correctness, since CSR
reads can have side effects or change between calls.

## Requirements

- A GCC- or Clang-compatible compiler (extended inline-asm syntax).
- A RISC-V target (`riscv32-*` or `riscv64-*`); the CSR instruction family
  is RISC-V–specific.
- `<stdint.h>` from a freestanding-capable toolchain (no libc dependency
  beyond the standard integer type definitions).