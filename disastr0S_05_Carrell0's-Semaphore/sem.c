#include <assert.h>
#include <stdlib.h>
#include "sem_structures.h"
#include "pool_allocator.h"
#include "disastrOS_globals.h"
#include "disastrOS_pcb.h"
#include "disastrOS_schedule.h"
#include "disastrOS_constants.h" 

static char             _sem_buffer[SEMAPHORE_BUFFER_SIZE];
static PoolAllocator    _sem_allocator;
static int              _next_sem_rid = 1;


void Semaphore_init(){
  int res = PoolAllocator_init(
    &_sem_allocator,
    SEMAPHORE_ITEM_SIZE, 
    SEMAPHORE_MAX_ITEMS,
    _sem_buffer,
    SEMAPHORE_BUFFER_SIZE
  );
  assert(res == 0);
}

/* Alloca e inizializza un semaforo */
Semaphore* Semaphore_alloc(int value){
  Semaphore* sem = (Semaphore*)PoolAllocator_getBlock(&_sem_allocator);
  if (!sem) return NULL;
  sem->value = value;
  sem->rid   = _next_sem_rid++;
  List_init(&sem->waiting);
 
  List_insert(&resources_list, resources_list.last, (ListItem*)sem);
  return sem;
}

/* Libera un semaforo */
int Semaphore_free(Semaphore* sem){
  while(sem->waiting.first) {
    ListItem* item = List_detach(&sem->waiting, sem->waiting.first);
    PCBPtr_free((PCBPtr*)item);
  }
  List_detach(&resources_list, (ListItem*)sem);
  return PoolAllocator_releaseBlock(&_sem_allocator, (void*)sem);
}

/* Trova un semaforo */
Semaphore* sem_by_rid(int rid){
  ListItem* it = resources_list.first;
  while(it){
    Semaphore* sem = (Semaphore*)it;
    if (sem->rid == rid) return sem;
    it = it->next;
  }
  return NULL;
}

/* crea un nuovo semaforo */
int Semaphore_open(int value){
  Semaphore* sem = Semaphore_alloc(value);
  if (!sem) return DSOS_ENOMEM;
  running->syscall_retvalue = sem->rid;
  return sem->rid;
}

/* Classic sem_wait: se il conteggio scende sotto zero, blocca il processo chiamante */
int Semaphore_wait(int rid){
  Semaphore* sem = sem_by_rid(rid);
  if (!sem) return DSOS_EINVAL;

  printf("[SEM_WAIT] pid=%d\tvalue(before)=%d\n", running->pid, sem->value);
  
  sem->value--;
  if (sem->value < 0) {

    printf("[SEM_WAIT] pid=%d\t-> BLOCKED, value(after)=%d\n", running->pid, sem->value);

    running->status = Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
    
    PCBPtr* pcb_ptr = PCBPtr_alloc(running);
    if (!pcb_ptr) {
      sem->value++;
      List_detach(&waiting_list, (ListItem*)running);
      running->status = Ready; 
      return DSOS_ENOMEM;
    }
    List_insert(&sem->waiting, sem->waiting.last, (ListItem*)pcb_ptr);

    internal_schedule();
    printf("[SEM_WAIT] pid=%d\t-> RESUMED\n", running->pid);
    
    return running->syscall_retvalue;
  }
  
  printf("[SEM_WAIT] pid=%d\t-> CONTINUES, value(after)=%d\n", running->pid, sem->value);
  return 0;
}


/* Classic sem_post: se ci sono processi in attesa, ne risveglia uno; altrimenti incrementa il conteggio */
int Semaphore_post(int rid) {
    Semaphore* sem = sem_by_rid(rid);
    if (!sem) return DSOS_EINVAL;

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
    
    else {
        printf("[SEM_POST] pid=%d\t-> INCREMENTED, value(after)=%d\n", running->pid, sem->value);
    }
    return 0;
}


/* Chiude il semaforo e libera le sue risorse */
int Semaphore_close(int rid){
  Semaphore* sem = sem_by_rid(rid);
  if (!sem) return DSOS_EINVAL;

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

  return Semaphore_free(sem);
}
