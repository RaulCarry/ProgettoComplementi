#pragma once
#include "linked_list.h"
#include "disastrOS_pcb.h"

typedef struct{
    ListItem list;
    int rid;
    int capacity;
    void** slots;
    int head,tail;
    int count;
    ListHead wait_send;
    ListHead wait_recv;
}Mqueue;

void MQueue_init();

void internal_mq_open();
void internal_mq_send();
void internal_mq_receive();
void internal_mq_close();