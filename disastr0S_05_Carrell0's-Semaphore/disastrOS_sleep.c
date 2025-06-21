#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_timer.h"

void internal_sleep(){
  if (running->timer) {
    printf("process has already a timer!!!!!\n");
    running->syscall_retvalue = DSOS_ESLEEP;
    return;
  }
  
  int cycles_to_sleep = running->syscall_args[0];
  if (cycles_to_sleep < 0) {
      running->syscall_retvalue = DSOS_EINVAL;
      return;
  }
  
  int wake_time = disastrOS_time + cycles_to_sleep;
  
  TimerItem* new_timer = TimerList_add(&timer_list, wake_time, running);
  if (!new_timer) {
    printf("Error: Could not create a new timer!!!!!!\n");
    running->syscall_retvalue = DSOS_ESLEEP;
    return;
  }
  
  running->timer = new_timer;
  running->status = Waiting;
  List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
  
  if (ready_list.first) {
    running = (PCB*) List_detach(&ready_list, ready_list.first);
    running->status = Running;
  } else {
    running = 0;
    disastrOS_debug("tutti i processi riposano o in idling, la sleep funziona ( ͡° ͜ʖ ͡°), prego attendere.\n");
  }
}