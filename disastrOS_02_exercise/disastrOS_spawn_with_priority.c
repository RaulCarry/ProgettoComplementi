#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

void internal_spawnWithPriority() {
  assert(running && "internal_spawnWithPriority|ERROR, no running");

#ifdef _VERBOSE_
  printf("internal_spawnWithPriority|spawining thread with priority [ %03ld ]\n",
         running->syscall_args[0]);
#endif

  //ia collect syscall arguments
  int t_quantums = (int)running->syscall_args[0];
  void (*t_fn) (void*)= (void(*)(void*))  running->syscall_args[1];
  void* t_fn_args = (void*) running->syscall_args[2];

  //ia check consistency of the arguments
  if (t_quantums < 1 || t_quantums > DSOS_MAX_PRIORITY) {
    running->syscall_retvalue = DSOS_ESPAWN_W_PRIORITY;
    return;
  }

  //ia create a new pcb
  PCB* new_pcb;
  new_pcb=PCB_alloc();
  if (!new_pcb) {
    running->syscall_retvalue=DSOS_ESPAWN_W_PRIORITY;
    return;
  }

  //ia standard PCB setup
  new_pcb->status=Ready;

  // sets the parent of the newly created process to the running process
  new_pcb->parent=running;
  
  // adds a pointer to the new process to the children list of running
  PCBPtr* new_pcb_ptr=PCBPtr_alloc(new_pcb);
  assert(new_pcb_ptr);
  List_insert(&running->children, running->children.last, (ListItem*) new_pcb_ptr);

  //adds the new process to the ready queue
  List_insert(&ready_list, ready_list.last, (ListItem*) new_pcb);

  //sets the retvalue for the caller to the new pid
  running->syscall_retvalue=new_pcb->pid;

  getcontext(&new_pcb->cpu_state);
  new_pcb->cpu_state.uc_stack.ss_sp = new_pcb->stack;
  new_pcb->cpu_state.uc_stack.ss_size = STACK_SIZE;
  new_pcb->cpu_state.uc_stack.ss_flags = 0;
  sigemptyset(&new_pcb->cpu_state.uc_sigmask);
  new_pcb->cpu_state.uc_link = &main_context;
  makecontext(&new_pcb->cpu_state, (void(*)())  t_fn, 1, t_fn_args);

  //ia now we should setup the priority stuff
  new_pcb->quantum_total = t_quantums;
  new_pcb->quantum_elapsed = 0;
  new_pcb->quantum_to_go = t_quantums;
  
  return;
}
