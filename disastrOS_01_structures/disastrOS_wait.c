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

    while (1) {
        ListItem* aux = running->children.first;
        PCBPtr* child_ptr = NULL;
        int child_found = 0;

        while (aux) {
            child_ptr = (PCBPtr*)aux;
            PCB* child_pcb = child_ptr->pcb;
            if (child_pcb->status == Zombie && (pid_to_wait == 0 || pid_to_wait == child_pcb->pid)) {
                child_found = 1;
                break;
            }
            aux = aux->next;
        }

        if (child_found) {
            PCB* zombie_pcb = child_ptr->pcb;
            
            if (retval_ptr) {
                *retval_ptr = zombie_pcb->return_value;
            }
            running->syscall_retvalue = zombie_pcb->pid;

            List_detach(&running->children, (ListItem*)child_ptr);
            PCBPtr_free(child_ptr);

            List_detach(&zombie_list, (ListItem*)zombie_pcb);
            PCB_free(zombie_pcb);
            
            return; 
        }

        if (running->children.size == 0) {
            running->syscall_retvalue = DSOS_EWAIT; 
            return;
        }

        running->status = Waiting;
        List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
        
        if (ready_list.first) {
            running = (PCB*)List_detach(&ready_list, ready_list.first);
            running->status = Running;
        } else {
            running = 0;
            disastrOS_printStatus();
            printf("TUTTI I PROCESSI IN INDLING, ATTENDIAMO...\n");
        }
        
        internal_schedule();
    }
}