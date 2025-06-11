#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

// replaces the running process with the next one in the ready list
void internal_schedule() {
  if (running) {
    //disastrOS_debug("PREEMPT - %d ->", running->pid);
   }
  // at least one process should be in the running queue
  // if not, no preemption occurs

  // we check the timers, to see if to wake up some process
  // we insert the processes in front the ready list
  TimerItem* elapsed_timer=0;
  
  //printf("SCHEDULE\n");
  while( (elapsed_timer=TimerList_current(&timer_list, disastrOS_time)) ){
    PCB* pcb_to_wake=elapsed_timer->pcb;
    List_detach(&waiting_list, (ListItem*) pcb_to_wake);
    pcb_to_wake->status=Ready;
    List_insert(&ready_list, ready_list.last, (ListItem*) pcb_to_wake);
    TimerList_removeCurrent(&timer_list);
  }

  
  PCB* aux=ready_list.first;
  PCB* min_pcb=0;
  while (aux) {
    aux->priority_current-=aux->priority_time_gain;
    if (aux->priority_current<0)
      aux->priority_current=0;

    if (! min_pcb || min_pcb->priority_current>aux->priority_current)
      min_pcb=aux;
    
    aux=(PCB*)aux->list.next;
  }

  if(min_pcb) {
    PCB* next_process=(PCB*) List_detach(&ready_list, min_pcb);
    running->status=Ready;
    running->priority_current=running->priority_default;
    List_insert(&ready_list, ready_list.last, (ListItem*) running);
    next_process->status=Running;
    running=next_process;
  }
   
  if (running) {
    //disastrOS_debug(" %d\n", running->pid);
  }
}
