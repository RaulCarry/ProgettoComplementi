#include <assert.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_schedule.h"
#include "disastrOS_timer.h"

void internal_exit() {
  /* 1. salva il valore di ritorno */
  running->return_value = running->syscall_args[0];

  /* 2. riassegna eventuali figli a init e invia loro SIGHUP */
  assert(init_pcb);
  while (running->children.first) {
    ListItem* item = List_detach(&running->children, running->children.first);
    List_insert(&init_pcb->children, init_pcb->children.last, item);

    PCB* child = ((PCBPtr*) item)->pcb;
    child->signals |= (DSOS_SIGHUP & child->signals_mask);
  }

  /* 3. marca Zombie e inserisce in zombie_list */
  running->status = Zombie;
  List_insert(&zombie_list, zombie_list.last, (ListItem*) running);

  /* 4. invia SIGCHLD al padre e, se stava in wait, lo risveglia */
  PCB* parent = running->parent;
  if (parent) {
    parent->signals |= (DSOS_SIGCHLD & parent->signals_mask);

    if (parent->status == Waiting &&
        parent->syscall_num == DSOS_CALL_WAIT &&
        (parent->syscall_args[0] == 0 ||          /* wait() */
         parent->syscall_args[0] == running->pid)) {

      /* rimuovi il padre dalla waiting_list e rimettilo in ready */
      List_detach(&waiting_list, (ListItem*) parent);
      parent->status = Ready;
      List_insert(&ready_list, ready_list.last, (ListItem*) parent);

      /* imposta i valori di ritorno della wait */
      parent->syscall_retvalue = running->pid;
      int* resloc = (int*) parent->syscall_args[1];
      if (resloc) *resloc = running->return_value;
    }
  }

  /* 5. libera eventuali timer pendenti appartenenti a questo processo */
  for (ListItem* it = timer_list.first; it; ) {
    TimerItem* t = (TimerItem*) it;
    it = it->next;
    if (t->pcb == running) {
      List_detach(&timer_list, (ListItem*) t);
      TimerItem_free(t);
    }
  }

  /* 6. passa il controllo a un altro processo (swapcontext nello scheduler) */
  internal_schedule();

  /* 7. se non esiste più alcun processo, torna al contesto principale */
  setcontext(&main_context);           /* non ritorna */
}