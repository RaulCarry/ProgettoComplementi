CC        = gcc
CCOPTS    = --std=gnu99 -Wall -Ofast -D_DISASTROS_DEBUG_
AR        = ar
RM        = rm -rf

HEADERS   = disastrOS.h \
            disastrOS_constants.h \
            disastrOS_globals.h \
            disastrOS_syscalls.h \
            linked_list.h \
            pool_allocator.h \
            sem_structures.h \
            disastrOS_timer.h \
            disastrOS_pcb.h \
            queue.h

OBJS      = pool_allocator.o \
            linked_list.o \
            disastrOS_timer.o \
            disastrOS_pcb.o \
            disastrOS.o \
            disastrOS_wait.o \
            disastrOS_fork.o \
            disastrOS_spawn.o \
            disastrOS_exit.o \
            disastrOS_sleep.o \
            disastrOS_shutdown.o \
            disastrOS_schedule.o \
            disastrOS_preempt.o \
            disastrOS_syscalls.o \
            sem.o \
            queue.o

LIBS      = libdisastrOS.a
BINS      = sem_test mq_test

.PHONY: all clean

all: $(LIBS) $(BINS)


%.o: %.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<


$(LIBS): $(OBJS)
	$(AR) -rcs $@ $^
	$(RM) $(OBJS)


sem_test: test_sem.c $(LIBS)
	$(CC) $(CCOPTS) -o $@ $< -L. -ldisastrOS

mq_test: test_mq.c $(LIBS)
	$(CC) $(CCOPTS) -o $@ $< -L. -ldisastrOS

clean:
	$(RM) *.o *~ $(LIBS) $(BINS)
