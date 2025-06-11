#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_schedule.h"
#include "disastrOS_timer.h"
#include "queue.h"

void internal_schedule() {
  PCB* old_running = running;                 // processo che cede la CPU

  /* 1. sveglia eventuali processi con timer scaduto */
  TimerItem* t;
  while ((t = TimerList_current(&timer_list, disastrOS_time))) {
    PCB* p = t->pcb;
    List_detach(&waiting_list, (ListItem*)p);
    p->status = Ready;
    List_insert(&ready_list, ready_list.last, (ListItem*)p);
    TimerList_removeCurrent(&timer_list);
  }

  /* 2. seleziona il prossimo processo pronto */
  if (!ready_list.first)
    return;                                   // nessuno da eseguire

  PCB* next = (PCB*)List_detach(&ready_list, ready_list.first);

  /* 3. rimetti in coda il processo uscente SOLO se è ancora Running */
  if (old_running && old_running->status == Running) {
    old_running->status = Ready;
    List_insert(&ready_list, ready_list.last, (ListItem*)old_running);
  }

  /* 4. passa al nuovo processo */
  next->status = Running;
  running      = next;

  /* 5. vero cambio di contesto */
  if (old_running) {
    if (swapcontext(&old_running->cpu_state, &running->cpu_state) < 0) {
      perror("swapcontext");
      exit(1);
    }
  } else {
    setcontext(&running->cpu_state);          // primo avvio, non ritorna
  }
}

