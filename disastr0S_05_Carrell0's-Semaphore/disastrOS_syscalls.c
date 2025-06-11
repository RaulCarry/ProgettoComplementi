#include "disastrOS_syscalls.h"
#include "sem_structures.h"
#include "queue.h"
#include "disastrOS_globals.h"

void internal_sem_open() {
    int init = running->syscall_args[0];
    int rid  = Semaphore_open(init);
    running->syscall_retvalue = rid;
 }

void internal_sem_wait() {
     int rid = running->syscall_args[0];
     running->syscall_retvalue = Semaphore_wait(rid);
}

void internal_sem_post() {
    int rid = running->syscall_args[0];
    running->syscall_retvalue = Semaphore_post(rid);
}

void internal_sem_close() {
    int rid = running->syscall_args[0];
    running->syscall_retvalue = Semaphore_close(rid);
}

void internal_mq_open() {
    int cap = running->syscall_args[0];
    running->syscall_retvalue = MQueue_open(cap);
}

void internal_mq_send() {
    int rid = running->syscall_args[0];
    void* msg = (void*)running->syscall_args[1];
    running->syscall_retvalue = MQueue_send(rid,msg);
}

void internal_mq_receive() {
    int rid = running->syscall_args[0];
    void** msg = (void**)running->syscall_args[1];
    running->syscall_retvalue = MQueue_receive(rid,msg);
}

void internal_mq_close() {
    int rid = running->syscall_args[0];
    running->syscall_retvalue = MQueue_close(rid);
}