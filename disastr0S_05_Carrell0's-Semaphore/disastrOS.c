#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_timer.h"
#include "disastrOS_schedule.h"
#include "sem_structures.h"
#include "queue.h"

FILE* log_file=NULL;
PCB* init_pcb;
PCB* running;
int last_pid = 0;
ListHead ready_list;
ListHead waiting_list;
ListHead zombie_list;
ListHead timer_list;
ListHead resources_list;

SyscallFunctionType syscall_vector[DSOS_MAX_SYSCALLS];
int syscall_numarg[DSOS_MAX_SYSCALLS];

ucontext_t interrupt_context;
ucontext_t trap_context;
ucontext_t main_context;
int shutdown_now=0;
char system_stack[STACK_SIZE];

sigset_t signal_set;
volatile int disastrOS_time=0;

void timerHandler(int j, siginfo_t *si, void *old_context) {
  if (running) {
    swapcontext(&running->cpu_state, &interrupt_context);
  } else {
    setcontext(&interrupt_context);
  }
}

void idle_routine() {
    sigset_t temp_mask;
    sigfillset(&temp_mask);
    sigdelset(&temp_mask, SIGALRM); 
    
    while(!running) {
        sigsuspend(&temp_mask); 
    }
}

void timerInterrupt(){
  disastrOS_time++;
  internal_schedule();
  if (running) {
    setcontext(&running->cpu_state);
  } else {
    idle_routine();
    setcontext(&running->cpu_state);
  }
}

void setupSignals(void) {
  struct sigaction act;
  act.sa_sigaction = timerHandler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGALRM);
  if(sigaction(SIGALRM, &act, NULL) != 0) perror("Signal handler");
  struct itimerval it;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = INTERVAL * 1000;
  it.it_value = it.it_interval;
  if (setitimer(ITIMER_REAL, &it, NULL) ) perror("setitimer");
}

void disastrOS_trap(){
  int syscall_num = running->syscall_num;
  if (syscall_num < 0 || syscall_num > DSOS_MAX_SYSCALLS || !syscall_vector[syscall_num]) {
    running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
  } else {
    (*syscall_vector[syscall_num])();
  }

  if (running && ready_list.first) {
    internal_schedule();
  }
  
  if (running) {
      setcontext(&running->cpu_state);
  } else {
    idle_routine();
    setcontext(&running->cpu_state);
  }
}

int disastrOS_syscall(int syscall_num, ...) {
  assert(running);
  va_list ap;
  if (syscall_num < 0 || syscall_num > DSOS_MAX_SYSCALLS)
    return DSOS_ESYSCALL_OUT_OF_RANGE;
  int nargs = syscall_numarg[syscall_num];
  va_start(ap, syscall_num);
  for (int i=0; i<nargs; ++i){
    running->syscall_args[i] = va_arg(ap, long int);
  }
  va_end(ap);
  running->syscall_num = syscall_num;
  swapcontext(&running->cpu_state, &trap_context);
  return running->syscall_retvalue;
}

void disastrOS_start(void (*f)(void*), void* f_args, char* logfile){
  PCB_init(); Timer_init(); Semaphore_init(); MQueue_init();

  for (int i=0; i<DSOS_MAX_SYSCALLS; ++i) syscall_vector[i]=0;

  syscall_vector[DSOS_CALL_PREEMPT]   = internal_preempt;   syscall_numarg[DSOS_CALL_PREEMPT]   = 0;

  syscall_vector[DSOS_CALL_FORK]      = internal_fork;      syscall_numarg[DSOS_CALL_FORK]      = 0;

  syscall_vector[DSOS_CALL_SPAWN]     = internal_spawn;     syscall_numarg[DSOS_CALL_SPAWN]     = 2;

  syscall_vector[DSOS_CALL_WAIT]      = internal_wait;      syscall_numarg[DSOS_CALL_WAIT]      = 2;

  syscall_vector[DSOS_CALL_EXIT]      = internal_exit;      syscall_numarg[DSOS_CALL_EXIT]      = 1;

  syscall_vector[DSOS_CALL_SHUTDOWN]  = internal_shutdown;  syscall_numarg[DSOS_CALL_SHUTDOWN]  = 0;

  syscall_vector[DSOS_CALL_SLEEP]     = internal_sleep;     syscall_numarg[DSOS_CALL_SLEEP]     = 1;

  syscall_vector[DSOS_CALL_SEM_OPEN]  = internal_sem_open;  syscall_numarg[DSOS_CALL_SEM_OPEN]  = 1;

  syscall_vector[DSOS_CALL_SEM_WAIT]  = internal_sem_wait;  syscall_numarg[DSOS_CALL_SEM_WAIT]  = 1;

  syscall_vector[DSOS_CALL_SEM_POST]  = internal_sem_post;  syscall_numarg[DSOS_CALL_SEM_POST]  = 1;

  syscall_vector[DSOS_CALL_SEM_CLOSE] = internal_sem_close; syscall_numarg[DSOS_CALL_SEM_CLOSE] = 1;

  syscall_vector[DSOS_CALL_MQ_OPEN]   = internal_mq_open;   syscall_numarg[DSOS_CALL_MQ_OPEN]   = 1;

  syscall_vector[DSOS_CALL_MQ_SEND]   = internal_mq_send;   syscall_numarg[DSOS_CALL_MQ_SEND]   = 2;

  syscall_vector[DSOS_CALL_MQ_RECEIVE]= internal_mq_receive;syscall_numarg[DSOS_CALL_MQ_RECEIVE]= 2;

  syscall_vector[DSOS_CALL_MQ_CLOSE]  = internal_mq_close;  syscall_numarg[DSOS_CALL_MQ_CLOSE]  = 1;

  List_init(&ready_list); List_init(&waiting_list); List_init(&zombie_list);
  List_init(&resources_list); List_init(&timer_list);

  getcontext(&main_context);
  if (shutdown_now) exit(0);

  getcontext(&trap_context);
  trap_context.uc_stack.ss_sp = system_stack;
  trap_context.uc_stack.ss_size = STACK_SIZE;
  sigemptyset(&trap_context.uc_sigmask);
  sigaddset(&trap_context.uc_sigmask, SIGALRM);
  trap_context.uc_stack.ss_flags = 0;
  trap_context.uc_link = &main_context;
  makecontext(&trap_context, disastrOS_trap, 0);

  interrupt_context = trap_context;
  interrupt_context.uc_link = &main_context;
  sigemptyset(&interrupt_context.uc_sigmask);
  makecontext(&interrupt_context, timerInterrupt, 0);

  running=PCB_alloc();
  running->status=Running;
  init_pcb=running;
  
  getcontext(&running->cpu_state);
  running->cpu_state.uc_stack.ss_sp = running->stack;
  running->cpu_state.uc_stack.ss_size = STACK_SIZE;
  running->cpu_state.uc_stack.ss_flags = 0;
  running->cpu_state.uc_link = &main_context;
  makecontext(&running->cpu_state, (void(*)()) f, 1, f_args);

  setupSignals();
  setcontext(&running->cpu_state);
}

// Syscall Wrappers
int disastrOS_fork(){ 
  return disastrOS_syscall(DSOS_CALL_FORK); 
  }

int disastrOS_wait(int pid, int *retval){ 
  return disastrOS_syscall(DSOS_CALL_WAIT, pid, retval); 
  }

void disastrOS_exit(int exitval) { 
  disastrOS_syscall(DSOS_CALL_EXIT, exitval); 
  }
void disastrOS_preempt() { 
  disastrOS_syscall(DSOS_CALL_PREEMPT); 
  }

void disastrOS_spawn(void (*f)(void*), void* args ) { 
  disastrOS_syscall(DSOS_CALL_SPAWN, f, args); 
  }


void disastrOS_shutdown() { 
  disastrOS_syscall(DSOS_CALL_SHUTDOWN); 
  }

void disastrOS_sleep(int sleep_time) { 
  disastrOS_syscall(DSOS_CALL_SLEEP, sleep_time); 
  }

int disastrOS_getpid(){ 
  if (!running) {
    return -1;
  }
  return running->pid; 
}

int disastrOS_sem_open(int v){ 
  return disastrOS_syscall(DSOS_CALL_SEM_OPEN, v); 
  }
int disastrOS_sem_wait(int r){ 
  return disastrOS_syscall(DSOS_CALL_SEM_WAIT, r); 
  }

void disastrOS_sem_post(int r){ 
  disastrOS_syscall(DSOS_CALL_SEM_POST, r); 
  }

void disastrOS_sem_close(int r){ 
  disastrOS_syscall(DSOS_CALL_SEM_CLOSE, r); 
  }

int disastrOS_mq_open(int capacity) { 
  return disastrOS_syscall(DSOS_CALL_MQ_OPEN, capacity);
   }
int disastrOS_mq_send(int rid, void* msg) { 
  return disastrOS_syscall(DSOS_CALL_MQ_SEND, rid, msg); }

int disastrOS_mq_receive(int rid, void** msg_ptr) { 
  return disastrOS_syscall(DSOS_CALL_MQ_RECEIVE, rid, msg_ptr); 
  }

int disastrOS_mq_close(int rid) { 
  return disastrOS_syscall(DSOS_CALL_MQ_CLOSE, rid);
   }

void disastrOS_printStatus(){
  printf("****************** DisastrOS ******************\n");
  printf("Running: ");
  if (running)
    PCB_print(running);
  printf("\n");
  printf("Timers: ");
  TimerList_print(&timer_list);
  printf("\nReady: ");
  PCBList_print(&ready_list);
  printf("\nWaiting: ");
  PCBList_print(&waiting_list);
  printf("\nZombie: ");
  PCBList_print(&zombie_list);
  printf("\n***********************************************\n\n");
};