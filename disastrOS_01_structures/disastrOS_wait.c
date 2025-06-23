#include <assert.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_pcb.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_globals.h"
#include "disastrOS_schedule.h"

void internal_wait() {
    int pid_to_wait = (int)running->syscall_args[0];
    int* retval_ptr = (int*)running->syscall_args[1];

    if (running->children.first == NULL) {
        running->syscall_retvalue = DSOS_EWAIT; 
        return;
    }

    ListItem* aux = running->children.first;
    PCBPtr* child_ptr_found = NULL;
    PCB* child_pcb_found = NULL;

    while (aux) {
        PCBPtr* child_ptr = (PCBPtr*)aux;
        PCB* pcb = child_ptr->pcb;
        if (pcb->status == Zombie && (pid_to_wait == 0 || pid_to_wait == pcb->pid)) {
            child_pcb_found = pcb;
            child_ptr_found = child_ptr;
            break;
        }
        aux = aux->next;
    }

    if (child_pcb_found) {
        if (retval_ptr) {
            *retval_ptr = child_pcb_found->return_value;
        }
        running->syscall_retvalue = child_pcb_found->pid;

        List_detach(&running->children, (ListItem*)child_ptr_found);
        PCBPtr_free(child_ptr_found);

        List_detach(&zombie_list, (ListItem*)child_pcb_found);
        PCB_free(child_pcb_found);
        
        return;
    }

    running->status = Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
    
    running = 0; 
    internal_schedule(); 
}
