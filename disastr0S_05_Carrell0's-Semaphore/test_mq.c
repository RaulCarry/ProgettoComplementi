#include <stdio.h>
#include "disastrOS.h"

#define CAPACITY      2 
#define N_PRODUCERS   2
#define N_CONSUMERS   2

static int mq_rid;


void producer(void* args) {
  int pid = disastrOS_getpid();
  int value = pid * 10;                       
  printf("[Prod  %d] send %d\n", pid, value);

  disastrOS_mq_send(mq_rid, (void*)(long)value);

  printf("[Prod  %d] done\n", pid);
  disastrOS_exit(0);
}


void consumer(void* args) {
  int pid = disastrOS_getpid();
  void* msg;
  printf("[Cons  %d] waiting…\n", pid);

  disastrOS_mq_receive(mq_rid, &msg);

  printf("[Cons  %d] got %ld\n", pid, (long)msg);
  disastrOS_exit((long)msg);
}

void initFunction(void* args) {
  printf("[Init] opening MQ (cap=%d)…\n", CAPACITY);
  mq_rid = disastrOS_mq_open(CAPACITY);
  printf("[Init] mq rid = %d\n", mq_rid);

  for (int i = 0; i < N_CONSUMERS; ++i)
    disastrOS_spawn(consumer, NULL);

  disastrOS_preempt();       

  for (int i = 0; i < N_PRODUCERS; ++i)
    disastrOS_spawn(producer, NULL);

  int alive = N_PRODUCERS + N_CONSUMERS;
  int pid, retval;
  while (alive && (pid = disastrOS_wait(0, &retval)) > 0) {
    printf("[Init] child %d terminated (rv=%d)\n", pid, retval);
    --alive;
  }

  disastrOS_mq_close(mq_rid);
  printf("[Init] MQ test completed, shutting down.\n");
  disastrOS_shutdown();

}


int main(int argc, char** argv) {
  printf("=== MQ Test ===\n");
  disastrOS_start(initFunction, NULL, NULL);
  return 0;
}