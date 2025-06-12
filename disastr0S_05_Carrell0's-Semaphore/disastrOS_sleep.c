#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

void internal_sleep(){

  if (running->timer) {
    printf("il processo ha già un timer!!!\n");
    running->syscall_retvalue=DSOS_ESLEEP;
    return;
  }
  int cycles_to_sleep=running->syscall_args[0];
  int wake_time=disastrOS_time+cycles_to_sleep;
  
  TimerItem* new_timer=TimerList_add(&timer_list, wake_time, running);
  if (! new_timer) {
    printf("nun je sta il timer!!!\n");
    running->syscall_retvalue=DSOS_ESLEEP;
    return;
  } 

  running->status=Waiting;

  List_insert(&waiting_list, waiting_list.last, (ListItem*) running);

}