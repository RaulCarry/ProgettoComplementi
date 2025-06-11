#include <stdio.h>
#include "disastrOS_syscalls.h"

//ia implementation of the syscall
void internal_revertAndPreempt() {
  assert(running && "internal_revertAndPreempt|ERROR, no running process\n");
  disastrOS_debug("internal_revertAndPreempt|current running [ %d ] -> ", running->pid);

  //ia check if ready list contains something -- TODO return an error
  if (!ready_list.size || !ready_list.first) {
    //ia nothing to do here
    running->syscall_retvalue = DSOS_EREVERT_AND_PREEMPT;
    return;
  }

  disastrOS_debug("reverting ready list -> ");

  ListHead new_ready_list;
  List_init(&new_ready_list);
  while (ready_list.first) {
    ListItem* current=List_popFront(&ready_list);
    List_pushFront(&new_ready_list, current);
  }
  ready_list=new_ready_list;
  running->syscall_retvalue = 0;
  running->status=Ready;
  List_pushBack(&ready_list, (ListItem*) running);
  running=(PCB*)List_popFront(&ready_list);
  running->status=Running;
  
  disastrOS_debug("new running [ %d ]\n", running->pid);
}
