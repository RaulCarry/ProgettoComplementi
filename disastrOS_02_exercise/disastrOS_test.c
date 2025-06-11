#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include "disastrOS.h"

#define A_BIT 100000000
#define A_BIT_MORE 2*A_BIT
#define TARGET_NUM_THREADS 2

void busyWait(int num_cicles) {
  for (int i=0; i<num_cicles; ++i);
}
// we need this to handle the sleep state
void sleeperFunction(void* args){
  printf("PID [ \033[33m %d \033[0m ]||Hello, I am the sleeper, and I sleep\n", disastrOS_getpid());
  while(1) {
    getc(stdin);
    printf("PID [ \033[33m %d \033[0m ]|log request\n", disastrOS_getpid());
    disastrOS_printStatus();
  }
}

void childFunction(void* args){
  printf("PID [ \033[33m %d \033[0m ]|Hello, I am the child function\n", disastrOS_getpid());
  const int iterations = disastrOS_getpid()+1;
  for (int i=0; i<iterations; ++i){
    printf("PID [ \033[33m %d \033[0m ]|iteration [ %03d / %03d ]\n",
           disastrOS_getpid(), i, iterations);
    busyWait(A_BIT);
  }
  
  printf("PID [ \033[33m %d \033[0m ]|terminating\n", disastrOS_getpid());
  disastrOS_exit(disastrOS_getpid());
}


void initFunction(void* args) {
  disastrOS_printStatus();
  printf("initFunction|hello, I am init and I just started\n");

  int spawn_result = 0;  
  printf("initFunction|spawning sleeper\n");
  spawn_result = disastrOS_spawnWithPriority(5, sleeperFunction, NULL);
  if (spawn_result == DSOS_ESPAWN_W_PRIORITY) {
    printf("initFunction|ERROR while spawining\n");
    exit(EXIT_FAILURE);
  }
  disastrOS_printStatus();
  printf("initFunction|spawning sleeper done\n");

  printf("initFunction|I feel like to spawn %d nice threads\n", TARGET_NUM_THREADS);
  int alive_children = 0;
  for (int i = 0; i < TARGET_NUM_THREADS; ++i) {
    spawn_result = disastrOS_spawnWithPriority(alive_children+1, childFunction, NULL);
    if (spawn_result == DSOS_ESPAWN_W_PRIORITY) {
      printf("initFunction|ERROR while spawining\n");
      exit(EXIT_FAILURE);
    }    
    alive_children++;
  }
  
  if (alive_children != TARGET_NUM_THREADS) {
    printf("initFunction|ERROR, something bad happened\n");
    exit(EXIT_FAILURE);
  }

  disastrOS_printStatus();
  printf("initFunction|alive children: %d\n", alive_children);

  printf("initFunction|PID [ \033[33m %d \033[0m ] waiting any child to die\n", disastrOS_getpid());
  int retval;
  int pid;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){
    printf("initFunction|current PID [ \033[33m %d \033[0m ]|terminated PID [ %d ]|retval [ %d ]|alive [ %d ]\n",
           disastrOS_getpid(), pid, retval, alive_children);
    disastrOS_printStatus();
    busyWait(A_BIT);
    --alive_children;
  }
  printf("initFunction|shutdown!\n");
  disastrOS_shutdown();
}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    logfilename=argv[1];
  }
  // we create the init process processes
  // the first is in the running variable
  // the others are in the ready queue
  printf("the function pointer is: %p", childFunction);
  // spawn an init process
  printf("start\n");
  disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
