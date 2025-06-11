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
int  MQueue_open(int capacity);                
int  MQueue_send(int rid, void* msg);          
int  MQueue_receive(int rid, void** msg);     
int  MQueue_close(int rid);




