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

FILE* log_file=NULL;
PCB* init_pcb;
PCB* running;
int last_pid;
ListHead ready_list;
ListHead waiting_list;
ListHead zombie_list;

SyscallFunctionType syscall_vector[DSOS_MAX_SYSCALLS];
int syscall_numarg[DSOS_MAX_SYSCALLS];

ucontext_t interrupt_context;           
ucontext_t trap_context;
ucontext_t main_context;
//ucontext_t idle_context;
int shutdown_now=0; // used for termination
char system_stack[STACK_SIZE];

sigset_t signal_set;                       // process wide signal mask 
volatile int trap_running=0;
volatile int disastrOS_time=0;

void timerHandler(int j, siginfo_t *si, void *old_context) {
  swapcontext(&running->cpu_state, &interrupt_context);
}

void timerInterrupt(){
  if (log_file)
    fprintf(log_file, "TIME: %d\tPID: %d\tACTION: %s\n", disastrOS_time, running->pid, "TIMER_OUT");
  ++disastrOS_time;
  //printf("time: %d\n", disastrOS_time);

  // call the scheduler!!!
  internal_schedule();

  if (log_file)
    fprintf(log_file, "TIME: %d\tPID: %d\tACTION: %s\n", disastrOS_time, running->pid, "TIMER_IN");
  setcontext(&running->cpu_state);
}


//set up the signal action
void setupSignals(void) {
  struct sigaction act;
  act.sa_sigaction = timerHandler;
 
  // restart the signal handler, and take the handler from the sa_sigaction field
  act.sa_flags = SA_RESTART | SA_SIGINFO; 

  // handle only sigalarm
  sigemptyset(&act.sa_mask);
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGALRM);

  // install the handler
  if(sigaction(SIGALRM, &act, NULL) != 0) {
    perror("Signal handler");
  }

  // start a system  timer that will raise a sigalarm each INTERVAL ms
  struct itimerval it;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = INTERVAL * 1000;
  it.it_value = it.it_interval;
  if (setitimer(ITIMER_REAL, &it, NULL) ) perror("setitiimer");
}




int disastrOS_syscall(int syscall_num, ...) {
  assert(running); 
  va_list ap;
  if (syscall_num<0||syscall_num>DSOS_MAX_SYSCALLS)
    return DSOS_ESYSCALL_OUT_OF_RANGE;

  // pack the arguments of the syscalls in the running pcb
  int nargs=syscall_numarg[syscall_num];
  va_start(ap,syscall_num);
  for (int i=0; i<nargs; ++i){
    running->syscall_args[i] = va_arg(ap,long int);
  }
  va_end(ap);
  running->syscall_num=syscall_num;

  // trap to system context
  swapcontext(&running->cpu_state, &trap_context);
  return running->syscall_retvalue;
}

void disastrOS_trap(){
  int syscall_num=running->syscall_num;
  if (log_file)
    fprintf(log_file, "TIME: %d\tPID: %d\tACTION: %s %d\n",
	    disastrOS_time,
	    running->pid,
	    "SYSCALL_IN",
	    syscall_num);
  
  if (syscall_num<0||syscall_num>DSOS_MAX_SYSCALLS) {
    running->syscall_retvalue = DSOS_ESYSCALL_OUT_OF_RANGE;
    goto return_to_process;
  }
  SyscallFunctionType my_syscall=syscall_vector[syscall_num];
  if (! my_syscall) {
    running->syscall_retvalue = DSOS_ESYSCALL_NOT_IMPLEMENTED;
    goto return_to_process;
  }
 
  disastrOS_debug("syscall: %d, pid: %d\n", syscall_num, running->pid);
  (*syscall_vector[syscall_num])();
 return_to_process:
  if (log_file)
    fprintf(log_file, "TIME: %d\tPID: %d\tACTION: %s %d\n",
	    disastrOS_time,
	    running->pid,
	    "SYSCALL_OUT",
	    syscall_num);
  if (running) {
    // trap to the process context saved before the syscall
    setcontext(&running->cpu_state);
  } else {
    printf("no active processes\n");
    disastrOS_printStatus();
  }
}

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

int disastrOS_getpid(){
  if (! running)
    return -1;
  return running->pid;
}

void disastrOS_start(void (*f)(void*), void* f_args, char* logfile){  
  /* INITIALIZATION OF SYSTEM STRUCTURES*/
  disastrOS_debug("initializing system structures\n");
  PCB_init();
  init_pcb=0;

  // populate the vector of syscalls and number of arguments for each syscall
  for (int i=0; i<DSOS_MAX_SYSCALLS; ++i){
    syscall_vector[i]=0;
  }
  syscall_vector[DSOS_CALL_PREEMPT]   = internal_preempt;
  syscall_numarg[DSOS_CALL_PREEMPT]   = 0;
  
  syscall_vector[DSOS_CALL_FORK]      = internal_fork;
  syscall_numarg[DSOS_CALL_FORK]      = 0;

  syscall_vector[DSOS_CALL_SPAWN]      = internal_spawn;
  syscall_numarg[DSOS_CALL_SPAWN]      = 2;

  syscall_vector[DSOS_CALL_WAIT]      = internal_wait;
  syscall_numarg[DSOS_CALL_WAIT]      = 2;

  syscall_vector[DSOS_CALL_EXIT]      = internal_exit;
  syscall_numarg[DSOS_CALL_EXIT]      = 1;

  syscall_vector[DSOS_CALL_SHUTDOWN]      = internal_shutdown;
  syscall_numarg[DSOS_CALL_SHUTDOWN]      = 0;

  // setup the scheduling lists
  running=0;
  List_init(&ready_list);
  List_init(&waiting_list);
  List_init(&zombie_list);
  //List_init(&resources_list);
  //List_init(&timer_list);


  /* INITIALIZATION OF SYSCALL AND INTERRUPT INFRASTRUCTIRE*/
  disastrOS_debug("setting entry point for system shudtown... ");
  getcontext(&main_context); //<< we will come back here on shutdown
  if (shutdown_now)
    exit(0);
  
  // setting system trap
  disastrOS_debug("setting entry point for system trap... ");
  getcontext(&trap_context);
  trap_context.uc_stack.ss_sp = system_stack;
  trap_context.uc_stack.ss_size = STACK_SIZE;
  sigemptyset(&trap_context.uc_sigmask);
  sigaddset(&trap_context.uc_sigmask, SIGALRM); // we mask sigalarm when handing a trap
  trap_context.uc_stack.ss_flags = 0;
  trap_context.uc_link = &main_context;
  makecontext(&trap_context, disastrOS_trap, 0); //<< this extablishes a context for the system

  disastrOS_debug("setting entry point for timer interrupt... ");
  interrupt_context=trap_context; // the interrupt and the system live on the same stack
  interrupt_context.uc_link = &main_context;
  sigemptyset(&interrupt_context.uc_sigmask);
  makecontext(&interrupt_context, timerInterrupt, 0); //< this is a context for the "interrupt"


  /* STARTING FIRST PROCESS AND IDLING*/
  running=PCB_alloc();
  running->status=Running;
  init_pcb=running;
  
  // create a trampoline for the first process (see spawn)
  disastrOS_debug("preparing trampoline for first process ... ");
  getcontext(&running->cpu_state);
  running->cpu_state.uc_stack.ss_sp = running->stack;
  running->cpu_state.uc_stack.ss_size = STACK_SIZE;
  running->cpu_state.uc_stack.ss_flags = 0;
  running->cpu_state.uc_link = &main_context;
  makecontext(&running->cpu_state, (void(*)()) f, 1, f_args);

  // initialize timers and signals
  setupSignals();
  
  // we start the first process
  disastrOS_debug("starting\n");
  if (logfile){
    log_file=fopen(logfile, "w");
    fprintf(log_file, "TIME: %d\tPID: -1\tACTION: START\n", disastrOS_time);
  }
  // jump to init
  setcontext(&running->cpu_state);
}


void disastrOS_printStatus(){
  printf("****************** DisastrOS ******************\n");
  printf("Running: ");
  if (running)
    PCB_print(running);
  printf("\n");
  printf("\nReady: ");
  PCBList_print(&ready_list);
  printf("\nWaiting: ");
  PCBList_print(&waiting_list);
  printf("\nZombie: ");
  PCBList_print(&zombie_list);
  printf("\n***********************************************\n\n");
};
