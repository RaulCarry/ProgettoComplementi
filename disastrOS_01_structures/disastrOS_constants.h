#pragma once

#define MAX_NUM_PROCESSES 1024
#define STACK_SIZE        16384
// signals
#define MAX_SIGNALS 32
#define DSOS_SIGCHLD 0x1
#define DSOS_SIGHUP  0x2

// errors
#define DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS -1
#define DSOS_ESYSCALL_NOT_IMPLEMENTED -2
#define DSOS_ESYSCALL_OUT_OF_RANGE -3
#define DSOS_EFORK   -4
#define DSOS_EWAIT   -5
#define DSOS_ESPAWN  -6
#define DSOS_ESLEEP  -7
#define DSOS_EINVAL  -8 
#define DSOS_ENOMEM  -12 
#define DSOS_EMQ_CLOSED -13
#define DSOS_ESEM_CLOSED -14


// syscall numbers
#define DSOS_MAX_SYSCALLS 32
#define DSOS_MAX_SYSCALLS_ARGS 8
#define DSOS_CALL_PREEMPT   1
#define DSOS_CALL_FORK      2
#define DSOS_CALL_WAIT      3
#define DSOS_CALL_EXIT      4
#define DSOS_CALL_SPAWN     5
#define DSOS_CALL_SLEEP     6
#define DSOS_CALL_SHUTDOWN  7

//sem syscalls
#define DSOS_CALL_SEM_OPEN   10
#define DSOS_CALL_SEM_WAIT   11
#define DSOS_CALL_SEM_POST   12
#define DSOS_CALL_SEM_CLOSE  13

//mqueue syscalls
#define DSOS_CALL_MQ_OPEN     20
#define DSOS_CALL_MQ_SEND     21
#define DSOS_CALL_MQ_RECEIVE  22
#define DSOS_CALL_MQ_CLOSE    23

// scheduling
#define ALPHA 0.5f
#define INTERVAL 10 // milliseconds for timer tick
