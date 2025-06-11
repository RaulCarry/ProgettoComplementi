#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#include "disastrOS.h"


void initFunction(void* args) {
  disastrOS_printStatus();
  // below a sequence of actions performed
  // by the running process
  // process switch might occur as a consequence of
  // a preempt action
  // or a system call

  
  //ia call the newly installed syscall - we should get an error :)
  int revert_and_preempt_retval = 0;
  printf("revert and preempt \n");
  revert_and_preempt_retval = disastrOS_revertAndPreempt();
  if (revert_and_preempt_retval == DSOS_EREVERT_AND_PREEMPT) {
    printf("ERROR, empty ready list\n");
  }
  disastrOS_printStatus();
  
  // now we are in init
  // we pretend to fork
  printf("fork ");
  int fork_result = disastrOS_fork();
  printf(" child pid: %d\n", fork_result);
  disastrOS_printStatus();

  // we are still in the parent process;
  // we switch context;
  printf("preempt \n");
  disastrOS_preempt();
  disastrOS_printStatus();

  //we exit from child process
  disastrOS_exit(-1);
  disastrOS_printStatus();

  //parent reads value from child
  int retval=0;
  int wait_result=disastrOS_wait(0, &retval);
  printf("returned pid:%d, value:%d \n", wait_result, retval);
  disastrOS_printStatus();

  // parent forks three times
  printf("fork ");
  fork_result = disastrOS_fork();
  printf(" child pid: %d\n", fork_result);
  disastrOS_printStatus();
  printf("fork ");
  fork_result = disastrOS_fork();
  printf(" child pid: %d\n", fork_result);
  disastrOS_printStatus();
  printf("fork ");
  fork_result = disastrOS_fork();
  printf(" child pid: %d\n", fork_result);
  disastrOS_printStatus();

  //ia call the newly installed syscall - no error this time
  printf("revert and preempt \n");
  revert_and_preempt_retval = disastrOS_revertAndPreempt();
  if (revert_and_preempt_retval == DSOS_EREVERT_AND_PREEMPT) {
    printf("ERROR, empty ready list");
  }
  disastrOS_printStatus();

  //ia shutdown
  printf("exit\n");
  disastrOS_exit(-1);
  disastrOS_printStatus();
  
  printf("exit\n");
  disastrOS_exit(-1);
  disastrOS_printStatus();
  
  printf("exit\n");
  disastrOS_exit(-1);
  disastrOS_printStatus();
  
  printf("wait any\n");
  wait_result=disastrOS_wait(0, &retval);
  disastrOS_printStatus();

  printf("wait any\n");
  wait_result=disastrOS_wait(0, &retval);
  disastrOS_printStatus();

  printf("wait any\n");
  wait_result=disastrOS_wait(0, &retval);
  disastrOS_printStatus();

  return;
}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    logfilename=argv[1];
  }
  // we create the init process processes
  // the first is in the running variable
  // the others are in the ready queue
  // spawn an init process
  printf("start\n");
  disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
