#pragma once
#include "disastrOS_pcb.h"
#include "disastrOS_globals.h"

#define MAX_QUEUE_SIZE 16
#define SEMAPHORE_ITEM_SIZE sizeof(Semaphore)
#define SEMAPHORE_MAX_ITEMS  MAX_NUM_PROCESSES
#define SEMAPHORE_BUFFER_SIZE \
        (SEMAPHORE_MAX_ITEMS * (SEMAPHORE_ITEM_SIZE + sizeof(int)))

typedef struct Semaphore{
    ListItem list;
    int rid;
    int value;
    ListHead waiting;
} Semaphore;

void Semaphore_init();
void internal_sem_open();
void internal_sem_wait();
void internal_sem_post();
void internal_sem_close();





