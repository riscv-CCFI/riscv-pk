// See LICENSE for license details.

#include "pk.h"
#include "config.h"
#include "syscall.h"
#include "mmap.h"

static void handle_instruction_access_fault(trapframe_t *tf)
{
  dump_tf(tf);
  panic("Instruction access fault!");
}

static void handle_load_access_fault(trapframe_t *tf)
{
  dump_tf(tf);
  panic("Load access fault!");
}

static void handle_store_access_fault(trapframe_t *tf)
{
  dump_tf(tf);
  panic("Store/AMO access fault!");
}

static void handle_illegal_instruction(trapframe_t* tf)
{
  dump_tf(tf);
  panic("An illegal instruction was executed!");
}

static void handle_breakpoint(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Breakpoint!");
}

static void handle_misaligned_fetch(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned instruction access!");
}

static void handle_misaligned_load(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned Load!");
}

static void handle_misaligned_store(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned AMO!");
}

static void segfault(trapframe_t* tf, uintptr_t addr, const char* type)
{
  dump_tf(tf);
  const char* who = (tf->status & SSTATUS_SPP) ? "Kernel" : "User";
  panic("%s %s segfault @ %p", who, type, addr);
}

static void handle_fault_fetch(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_EXEC) != 0)
    segfault(tf, tf->badvaddr, "fetch");
}

static void handle_fault_load(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_READ) != 0)
    segfault(tf, tf->badvaddr, "load");
}

static void handle_fault_store(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_WRITE) != 0)
    segfault(tf, tf->badvaddr, "store");
}

static void handle_syscall(trapframe_t* tf)
{
  tf->gpr[10] = do_syscall(tf->gpr[10], tf->gpr[11], tf->gpr[12], tf->gpr[13],
                           tf->gpr[14], tf->gpr[15], tf->gpr[17]);
  tf->epc += 4;
}

/* RetTag: begin */
static void handle_rocc_interrupt(trapframe_t* tf)
{
  panic("[RoCC Interrupt] Authentication Failure"); 
}
/* RetTag: end */


static void handle_interrupt(trapframe_t* tf)
{
  /* RetTag: begin */
  typedef void (*trap_handler1)(trapframe_t*);
  const static trap_handler1 trap_handlers1[] = {
    [CAUSE_FETCH_PAGE_FAULT] = handle_rocc_interrupt,
  };

  trap_handler1 f = (void*)pa2kva(trap_handlers1[tf->cause&0x0ff]);

  f(tf);
  /* RetTag: end */  
  clear_csr(sip, SIP_SSIP);
}

void handle_trap(trapframe_t* tf)
{
  if ((intptr_t)tf->cause < 0)
    return handle_interrupt(tf);

  typedef void (*trap_handler)(trapframe_t*);

  const static trap_handler trap_handlers[] = {
    [CAUSE_MISALIGNED_FETCH] = handle_misaligned_fetch,
    [CAUSE_FETCH_ACCESS] = handle_instruction_access_fault,
    [CAUSE_LOAD_ACCESS] = handle_load_access_fault,
    [CAUSE_STORE_ACCESS] = handle_store_access_fault,
    [CAUSE_FETCH_PAGE_FAULT] = handle_fault_fetch,
    [CAUSE_ILLEGAL_INSTRUCTION] = handle_illegal_instruction,
    [CAUSE_USER_ECALL] = handle_syscall,
    [CAUSE_BREAKPOINT] = handle_breakpoint,
    [CAUSE_MISALIGNED_LOAD] = handle_misaligned_load,
    [CAUSE_MISALIGNED_STORE] = handle_misaligned_store,
    [CAUSE_LOAD_PAGE_FAULT] = handle_fault_load,
    [CAUSE_STORE_PAGE_FAULT] = handle_fault_store,
  };

  kassert(tf->cause < ARRAY_SIZE(trap_handlers) && trap_handlers[tf->cause]);

  trap_handler f = (void*)pa2kva(trap_handlers[tf->cause]);

  f(tf);
}
