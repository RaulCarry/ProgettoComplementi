CC=gcc
CCOPTS=--std=gnu99 -Wall 
AR=ar

HEADERS=disastrOS.h\
	disastrOS_constants.h\
	disastrOS_globals.h\
	disastrOS_pcb.h\
	disastrOS_syscalls.h\
	linked_list.h\
	pool_allocator.h\

OBJS=pool_allocator.o\
     linked_list.o\
     disastrOS_pcb.o\
     disastrOS.o\
     disastrOS_wait.o\
     disastrOS_fork.o\
     disastrOS_spawn.o\
     disastrOS_exit.o\
     disastrOS_shutdown.o\
     disastrOS_schedule.o\
     disastrOS_preempt.o\
     disastrOS_revertAndPreempt.o

LIBS=libdisastrOS.a

BINS=disastrOS_test

#disastros_test

.phony: clean all


all:	$(LIBS) $(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

libdisastrOS.a: $(OBJS) 
	$(AR) -rcs $@ $^
	$(RM) $(OBJS)

disastrOS_test:		disastrOS_test.c $(LIBS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(LIBS) $(BINS)
