#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

// replaces the running process with the next one in the ready list
void internal_schedule() {
  //ia this stuff should not happen ever
  assert(running && "internal_schedule|ERROR, no running\n");
  assert(running->quantum_to_go > -1 &&
         running->quantum_elapsed <= running->quantum_total &&
         "internal_schedule|ERROR, sick running process!");

#ifdef _VERBOSE_
  printf("internal_schedule|running pre: \033[33m");
  PCB_print(running);
  printf("\033[0m \n");
#endif
  
  if (running) {
    disastrOS_debug("PREEMPT - %d ->", running->pid);
  }

  //ia check if running has ended the number of quantums and eventually reset the counter
  if (!running->quantum_to_go) {
    running->quantum_elapsed = 0;
    running->quantum_to_go = running->quantum_total;
  }

  
  //ia instead of getting the first guy in the ready list
  //ia we navigate in it and we pick the guy with highest priority
  if (ready_list.first) {
#ifdef _VERBOSE_
    printf("internal_schedule|scanning ready list\n");
#endif
    
    //ia invalid initialization
    /* int min_quantum_to_go = DSOS_MAX_PRIORITY; //ia SRJF */
    int max_priority = -1; 
    PCB* next_process = 0;
    
    ListItem* aux = ready_list.first;
    while (aux) {
      PCB* p = (PCB*)aux;

#ifdef _VERBOSE_
      printf("\t");
      PCB_print(p);
      printf("\n");
#endif

      //ia SRJF
      /* if (p->quantum_to_go < min_quantum_to_go) { */
      /*   min_quantum_to_go = p->quantum_to_go; */
      /*   next_process = p; */
      /* } */

      if (p->quantum_to_go > max_priority) {
        max_priority = p->quantum_to_go;
        next_process = p;
      }
      aux = aux->next;
    }

    //ia we have our next guy - put it into running
    assert(next_process && "internal_schedule|ERROR ready list is sick");
#ifdef _VERBOSE_
    printf("internal_schedule|best process:");
    PCB_print(next_process);
    printf("\n");
#endif

    //ia detach the guy
    next_process=(PCB*) List_detach(&ready_list, (ListItem*)next_process);
    running->status=Ready;
    List_insert(&ready_list, ready_list.last, (ListItem*) running);
    next_process->status=Running;
    running=next_process;
  }

  //ia whoever is the new running will consume a CPU quantum
  --running->quantum_to_go;
  running->quantum_elapsed = running->quantum_total - running->quantum_to_go;

#ifdef _VERBOSE_
  printf("internal_schedule|running post: \033[32m");
  PCB_print(running);
  printf("\033[0m \n");
#endif
 
  if (running) {
    disastrOS_debug(" %d\n", running->pid);
  }
}
