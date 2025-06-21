#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_globals.h"
#include "queue.h"
#include "pool_allocator.h"
#include "disastrOS_constants.h"
#include "disastrOS_pcb.h"

#define MQ_SIZE    sizeof(Mqueue)
#define MQ_MAX     MAX_NUM_PROCESSES
#define MQ_MEMSIZE (MQ_SIZE + sizeof(int))

static char             _mq_buffer[MQ_MAX * MQ_MEMSIZE];
static PoolAllocator    _mq_allocator;
static int              last_mq_rid=100;

static Mqueue* mq_by_rid(int rid);

void MQueue_init(){
    int res = PoolAllocator_init(
        &_mq_allocator, MQ_SIZE, MQ_MAX,
        _mq_buffer, sizeof(_mq_buffer) );
    assert(res == 0);
}

static Mqueue* mq_by_rid(int rid) {
    ListItem* current_item = resources_list.first;
    while (current_item) {
        Mqueue* mq = (Mqueue*)current_item;
        if (mq->rid == rid) return mq;
        current_item = current_item->next;
    }
    return NULL;
}

void internal_mq_open(){
    int capacity = running->syscall_args[0];
    Mqueue* mq=(Mqueue*) PoolAllocator_getBlock(&_mq_allocator);
    if(!mq) {
        running->syscall_retvalue = DSOS_ENOMEM;
        return;
    }
    mq->rid=last_mq_rid++;
    mq->capacity=capacity;
    mq->slots=malloc(sizeof(void*)*capacity);
    mq->head=mq->tail=mq->count=0;
    List_init(&mq->wait_send);
    List_init(&mq->wait_recv);
    List_insert(&resources_list,resources_list.last,(ListItem*)mq);
    running->syscall_retvalue = mq->rid;
}

void internal_mq_send() {
    int rid = running->syscall_args[0];
    void* msg = (void*)running->syscall_args[1];
    Mqueue* mq = mq_by_rid(rid);

    if (!mq) {
        running->syscall_retvalue = DSOS_EINVAL;
        return;
    }

    if (mq->wait_recv.first) {
        ListItem* item = List_detach(&mq->wait_recv, mq->wait_recv.first);
        PCBPtr* pcb_ptr = (PCBPtr*)item;
        PCB* pcb_to_wake = pcb_ptr->pcb;

        void** consumer_msg_ptr = (void**)pcb_to_wake->syscall_args[1];

        *consumer_msg_ptr = msg;

        List_detach(&waiting_list, (ListItem*)pcb_to_wake);
        pcb_to_wake->status = Ready;
        pcb_to_wake->syscall_retvalue = 0; 
        List_insert(&ready_list, ready_list.last, (ListItem*)pcb_to_wake);
        
        PCBPtr_free(pcb_ptr);
        running->syscall_retvalue = 0; 
        return;
    }

    
    if (mq->count >= mq->capacity) {
        running->status = Waiting;
        List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
        PCBPtr* pcb_ptr = PCBPtr_alloc(running);
        List_insert(&mq->wait_send, mq->wait_send.last, (ListItem*)pcb_ptr);

        if (ready_list.first) {
            running = (PCB*) List_detach(&ready_list, ready_list.first);
            running->status = Running;
        } else {
            running = 0;
        }
        return;
    }

    
    mq->slots[mq->tail] = msg;
    mq->tail = (mq->tail + 1) % mq->capacity;
    mq->count++;

    running->syscall_retvalue = 0;
}

void internal_mq_receive() {
    int rid = running->syscall_args[0];
    void** msg = (void**)running->syscall_args[1];
    Mqueue* mq = mq_by_rid(rid);
    if (!mq) {
        running->syscall_retvalue = DSOS_EINVAL;
        return;
    }

    if (mq->count <= 0) {
        running->status = Waiting;
        List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
        PCBPtr* pcb_ptr = PCBPtr_alloc(running);
        List_insert(&mq->wait_recv, mq->wait_recv.last, (ListItem*)pcb_ptr);
        
        if (ready_list.first) {
            running = (PCB*) List_detach(&ready_list, ready_list.first);
            running->status = Running;
        } else {
            running = 0;
        }
        return;
    }

    *msg = mq->slots[mq->head];
    mq->head = (mq->head + 1) % mq->capacity;
    mq->count--;

    if (mq->wait_send.first) {
        ListItem* item = List_detach(&mq->wait_send, mq->wait_send.first);
        PCBPtr* pcb_ptr = (PCBPtr*)item;
        PCB* pcb_to_wake = pcb_ptr->pcb;

        List_detach(&waiting_list, (ListItem*)pcb_to_wake);
        pcb_to_wake->status = Ready;
        pcb_to_wake->syscall_retvalue = 0;
        List_insert(&ready_list, ready_list.last, (ListItem*)pcb_to_wake);
        PCBPtr_free(pcb_ptr);
    }
    running->syscall_retvalue = 0;
}

void internal_mq_close() {
    int rid = running->syscall_args[0];
    Mqueue* mq = mq_by_rid(rid);
    if (!mq) {
        running->syscall_retvalue = DSOS_EINVAL;
        return;
    }

    while (mq->wait_send.first) {
        ListItem* item = List_detach(&mq->wait_send, mq->wait_send.first);
        PCBPtr* pcb_ptr = (PCBPtr*)item;
        PCB* p = pcb_ptr->pcb;
        List_detach(&waiting_list, (ListItem*)p);
        p->status = Ready;
        p->syscall_retvalue = DSOS_EMQ_CLOSED;
        List_insert(&ready_list, ready_list.last, (ListItem*)p);
        PCBPtr_free(pcb_ptr);
    }

    while (mq->wait_recv.first) {
        ListItem* item = List_detach(&mq->wait_recv, mq->wait_recv.first);
        PCBPtr* pcb_ptr = (PCBPtr*)item;
        PCB* p = pcb_ptr->pcb;
        List_detach(&waiting_list, (ListItem*)p);
        p->status = Ready;
        p->syscall_retvalue = DSOS_EMQ_CLOSED;
        List_insert(&ready_list, ready_list.last, (ListItem*)p);
        PCBPtr_free(pcb_ptr);
    }

    free(mq->slots);
    List_detach(&resources_list, (ListItem*)mq);
    PoolAllocator_releaseBlock(&_mq_allocator, mq);
    running->syscall_retvalue = 0;
}