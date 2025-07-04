#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

// called upon termination
// moves the process to a zombie status
// if the process has children they need to be reparented to
// the init process (pid 1);
// all children should receive SIGHUP
// parent should receive SIGCHLD and be awakened if it was waiting
// for this process to report its value
void internal_exit(){
  // 1st register in pcb contains the exit value
  running->return_value=running->syscall_args[0];
    
  assert(init_pcb);
  while(running->children.first){

    // detach from current list
    ListItem* item = List_detach(& running->children, running->children.first);
    assert(item);
    
    // attach to init's children list
    List_insert(& init_pcb->children, init_pcb->children.last, item);

    // send SIGHUP
    PCBPtr* pcb_ptr=(PCBPtr*) item;
    PCB* pcb=pcb_ptr->pcb;
    pcb->signals |= (DSOS_SIGHUP & pcb->signals_mask);
  }

  running->status=Zombie;
  List_insert(&zombie_list, zombie_list.last, (ListItem*) running);
  running->parent->signals |= (DSOS_SIGCHLD & running->parent->signals_mask);

  // if the parent was waiting for this process to die
  if (running->parent->status==Waiting
  // since he called  wait or waitpid, we wake him up
      && running->parent->syscall_num==DSOS_CALL_WAIT
      && (running->parent->syscall_args[0]==0 ||
	  running->parent->syscall_args[0]==running->pid)
      ){
    PCB* parent= (PCB*) List_detach(&waiting_list, (ListItem*) running->parent);
    assert(parent);
    parent->status=Running;

    // we remove the process from the parent's children list
    PCBPtr* self_in_parent=PCBPtr_byPID(&parent->children, running->pid);
    List_detach(&parent->children, (ListItem*) self_in_parent);
    
    // hack the PCB of the parent to put in return value
    parent->syscall_retvalue=running->pid;
    int* result=(int*)parent->syscall_args[1];
    if (result)
      *result=running->return_value;
    
    // the process finally dies
    ListItem* suppressed_item = List_detach(&zombie_list, (ListItem*) running);
    PCB_free((PCB*) suppressed_item);
    running=parent;
  } else {
    // we put the first ready process in running 
    PCB* next_running=(PCB*) List_detach(&ready_list, ready_list.first);
    assert(next_running);
    next_running->status=Running;
    running=next_running;
  }
}
