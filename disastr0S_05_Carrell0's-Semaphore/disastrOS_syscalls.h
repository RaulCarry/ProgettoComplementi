#pragma once
#include <assert.h>
#include "disastrOS.h"
#include "disastrOS_globals.h"

void internal_preempt();
void internal_spawn();
void internal_wait();
void internal_exit();
void internal_shutdown();
void internal_fork();
void internal_sleep(); //La sleeep :-)

//sem
void internal_sem_open();
void internal_sem_wait();
void internal_sem_post();
void internal_sem_close();

//mqeueu
void internal_mq_open();
void internal_mq_send();
void internal_mq_receive();
void internal_mq_close();

