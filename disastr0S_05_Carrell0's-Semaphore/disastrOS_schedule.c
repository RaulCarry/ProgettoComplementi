#include <assert.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_schedule.h"
#include "disastrOS_timer.h"

void internal_schedule() {
  TimerItem* t;
  while ((t = TimerList_current(&timer_list, disastrOS_time))) {
    PCB* p = t->pcb;
    p->timer = 0;
    List_detach(&waiting_list, (ListItem*)p);
    p->status = Ready;
    List_insert(&ready_list, ready_list.last, (ListItem*)p);
    TimerList_removeCurrent(&timer_list);
  }

  if (!running && ready_list.first) {
    running = (PCB*) List_detach(&ready_list, ready_list.first);
    running->status = Running;
    return;
  }
  
  if (running && ready_list.first) {
      if (running->status != Running) {
        running = (PCB*) List_detach(&ready_list, ready_list.first);
        running->status = Running;
      } else {
        running->status = Ready;
        List_insert(&ready_list, ready_list.last, (ListItem*) running);
        running = (PCB*) List_detach(&ready_list, ready_list.first);
        running->status = Running;
      }
  }
}