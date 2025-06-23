#include <stdio.h>
#include "disastrOS.h"

#define N_WAITERS  2
#define N_POSTERS  2

static int sem_rid;

void waiter(void* args) {
  int pid = disastrOS_getpid();

  printf("[Waiter %d] attendiamo il semaforo: %d\n", pid, sem_rid);
  fflush(stdout);

  int ret = disastrOS_sem_wait(sem_rid);
  printf("[Waiter %d] ripreso (ret=%d)\n", pid, ret);
  
  fflush(stdout);
  disastrOS_exit(ret);
}

void poster(void* args) {
  int pid = disastrOS_getpid();
  disastrOS_sleep(1000*pid);
  printf("[Poster %d] postiamo il semaphore %d\n", pid, sem_rid);
  fflush(stdout);
  disastrOS_sem_post(sem_rid);
  
  disastrOS_exit(0);
}

void initFunction(void* args) {
  printf("[Init] apertura semaphore value 0…\n");
  fflush(stdout);
  sem_rid = disastrOS_sem_open(0);
  printf("[Init] sem rid = %d\n", sem_rid);
  fflush(stdout);

  for (int i = 0; i < N_WAITERS; ++i)
    disastrOS_spawn(waiter, NULL);

  disastrOS_preempt();

  for (int i = 0; i < N_POSTERS; ++i)
    disastrOS_spawn(poster, NULL);

  int alive = N_WAITERS + N_POSTERS;


  int pid, retval;
  while (alive && (pid = disastrOS_wait(0, &retval)) > 0) {
    printf("[Init] child %d terminato with retval=%d\n", pid, retval);
    fflush(stdout);
    --alive;
  }

  disastrOS_sem_close(sem_rid);
  printf("[Init] test completato, terminazione.\n");
  fflush(stdout);
  disastrOS_shutdown();
}

int main(int argc, char** argv) {
  printf("=== Sem Test ===\n");
   setbuf(stdout, NULL);
   disastrOS_start(initFunction, NULL, NULL);
  return 0;
}