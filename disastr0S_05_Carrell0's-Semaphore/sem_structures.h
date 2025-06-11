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
Semaphore* Semaphore_alloc(int value);
int Semaphore_open(int init_value);
int Semaphore_wait(int rid);
int Semaphore_post(int rid);
int Semaphore_close(int rid);







