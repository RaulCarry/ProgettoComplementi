#include <assert.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_schedule.h"

/* attende un figlio (pid=0 qualunque) e restituisce il suo pid;
   se result!=NULL scrive lì il valore di ritorno del figlio               */
void internal_wait() {
  int pid_req   = running->syscall_args[0];      /* 0 = wait()        */
  int* retval_p = (int*) running->syscall_args[1];

  /* errore immediato se non ho figli */
  if (!running->children.first) {
    running->syscall_retvalue = DSOS_EWAIT;
    return;
  }

  /* ciclo: cerca uno zombie; se non c’è, blocca e riprova alla riattivazione */
  for (;;) {

    /* 1. cerca un figlio zombie che soddisfi la richiesta */
    PCB*    zombie     = NULL;
    PCBPtr* zombie_ptr = NULL;
    int     seen_pid   = 0;                       /* per pid specifico */

    for (ListItem* it = running->children.first; it; it = it->next) {
      PCBPtr* cptr = (PCBPtr*) it;
      PCB*    cpcb = cptr->pcb;

      if (pid_req == 0 || cpcb->pid == pid_req)
        seen_pid = 1;                             /* esiste un figlio con quel pid */

      if ((pid_req == 0 || cpcb->pid == pid_req) &&
          cpcb->status == Zombie) {
        zombie     = cpcb;
        zombie_ptr = cptr;
        break;
      }
    }

    /* 2. se cercavo un pid che non è nemmeno mio figlio → errore */
    if (pid_req > 0 && !seen_pid) {
      running->syscall_retvalue = DSOS_EWAIT;
      return;
    }

    /* 3. se ho trovato uno zombie, “reap-pa” e restituisci */
    if (zombie) {
      /* stacca da children */
      List_detach(&running->children, (ListItem*) zombie_ptr);
      PCBPtr_free(zombie_ptr);

      /* stacca da zombie_list */
      List_detach(&zombie_list, (ListItem*) zombie);

      /* valori di ritorno */
      running->syscall_retvalue = zombie->pid;
      if (retval_p)
        *retval_p = zombie->return_value;

      PCB_free(zombie);
      return;
    }

    /* 4. nessuno zombie valido: blocca il padre e cedi la CPU */
    running->status = Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);

    internal_schedule();          /* swapcontext: quando torno ricomincio il for */
  }
}