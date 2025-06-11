#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_schedule.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_schedule.h"


void internal_preempt() {
  internal_schedule();
}
