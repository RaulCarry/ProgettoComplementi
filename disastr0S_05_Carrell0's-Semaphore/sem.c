#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "sem_structures.h"
#include "pool_allocator.h"
#include "disastrOS_globals.h"
#include "disastrOS_pcb.h"
#include "disastrOS_constants.h"


static char             _sem_buffer[SEMAPHORE_BUFFER_SIZE];
static PoolAllocator    _sem_allocator;
static int              _next_sem_rid = 1;


static Semaphore* Semaphore_alloc(int value);
static int Semaphore_free(Semaphore* sem);
static Semaphore* sem_by_rid(int rid);

void Semaphore_init(){
  int res = PoolAllocator_init(
    &_sem_allocator, SEMAPHORE_ITEM_SIZE, SEMAPHORE_MAX_ITEMS,
    _sem_buffer, SEMAPHORE_BUFFER_SIZE );
  assert(res == 0);
}

static Semaphore* Semaphore_alloc(int value){
  Semaphore* sem = (Semaphore*)PoolAllocator_getBlock(&_sem_allocator);
  if (!sem) return NULL;
  sem->value = value;
  sem->rid   = _next_sem_rid++;
  List_init(&sem->waiting);
  List_insert(&resources_list, resources_list.last, (ListItem*)sem);
  return sem;
}

static int Semaphore_free(Semaphore* sem){
  while(sem->waiting.first) {
    ListItem* item = List_detach(&sem->waiting, sem->waiting.first);
    PCBPtr_free((PCBPtr*)item);
  }
  List_detach(&resources_list, (ListItem*)sem);
  return PoolAllocator_releaseBlock(&_sem_allocator, (void*)sem);
}

static Semaphore* sem_by_rid(int rid){
  ListItem* it = resources_list.first;
  while(it){
    Semaphore* sem = (Semaphore*)it;
    if (sem->rid == rid) return sem;
    it = it->next;
  }
  return NULL;
}

void internal_sem_open(){
  int value = running->syscall_args[0];
  Semaphore* sem = Semaphore_alloc(value);
  if (!sem) {
      running->syscall_retvalue = DSOS_ENOMEM;
      return;
  }
  running->syscall_retvalue = sem->rid;
}

void internal_sem_wait(){
  int rid = running->syscall_args[0];
  Semaphore* sem = sem_by_rid(rid);
  if (!sem) {
      running->syscall_retvalue = DSOS_EINVAL;
      return;
  }

  printf("[SEM_WAIT] pid=%d\tvalue(before)=%d\n", running->pid, sem->value);
  sem->value--;

  if (sem->value < 0) {
    printf("[SEM_WAIT] pid=%d\t-> BLOCKED, value(after)=%d\n", running->pid, sem->value);
    running->status = Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
    PCBPtr* pcb_ptr = PCBPtr_alloc(running);
    List_insert(&sem->waiting, sem->waiting.last, (ListItem*)pcb_ptr);

    if (ready_list.first) {
        running = (PCB*) List_detach(&ready_list, ready_list.first);
        running->status = Running;
        printf("DEBUG: Processo bloccato, scelgo il prossimo processo da eseguire: PID %d\n", running->pid);
    } else {
        running = 0;
        printf("DEBUG: Processo bloccato, non ci sono processi pronti. Vado in IDLE.\n");
    }
    return;
  }

  running->syscall_retvalue = 0;
}

void internal_sem_post() {
    int rid = running->syscall_args[0];
    Semaphore* sem = sem_by_rid(rid);
    if (!sem) {
      running->syscall_retvalue = DSOS_EINVAL;
      return;
    }

    printf("[SEM_POST] pid=%d\tvalue(before)=%d\n", running->pid, sem->value);
    sem->value++;

    if (sem->value <= 0) {
        ListItem* item = List_detach(&sem->waiting, sem->waiting.first);
        if (item) {
            PCBPtr* pcb_ptr = (PCBPtr*)item;
            PCB* pcb_to_wake = pcb_ptr->pcb;
            printf("[SEM_POST] pid=%d\t-> WAKES pid=%d\n", running->pid, pcb_to_wake->pid);
            List_detach(&waiting_list, (ListItem*)pcb_to_wake);
            pcb_to_wake->status = Ready;
            pcb_to_wake->syscall_retvalue = 0;
            List_insert(&ready_list, ready_list.last, (ListItem*)pcb_to_wake);
            PCBPtr_free(pcb_ptr);
        }
    }
    running->syscall_retvalue = 0;
}

void internal_sem_close(){
  int rid = running->syscall_args[0];
  Semaphore* sem = sem_by_rid(rid);
  if (!sem) {
    running->syscall_retvalue = DSOS_EINVAL;
    return;
  }

  while(sem->waiting.first) {
      ListItem* item = List_detach(&sem->waiting, sem->waiting.first);
      PCBPtr* pcb_ptr = (PCBPtr*)item;
      PCB* pcb_to_wake = pcb_ptr->pcb;
      List_detach(&waiting_list, (ListItem*)pcb_to_wake);
      pcb_to_wake->status = Ready;
      pcb_to_wake->syscall_retvalue = DSOS_ESEM_CLOSED;
      List_insert(&ready_list, ready_list.last, (ListItem*)pcb_to_wake);
      PCBPtr_free(pcb_ptr);
  }
  Semaphore_free(sem);
  running->syscall_retvalue = 0;
}