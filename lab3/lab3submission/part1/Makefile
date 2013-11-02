CC=mpicc
CFLAGS=-Wall -g
INCLUDE=-I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
#LIBS=-lglib-2.0
RUN=mpirun
HOSTS=--hostfile ../../../hosts
n=10
PART1_SOURCE=master.c\
             mw_api.c\
             slave.c\
			 linked_list.c\
			 supervisor.c\
			 test_part1.c
PART1_OBJECTS=$(PART1_SOURCE:.c=.o)
PART1_EXEC=part1
DEBUG=1

all:
	echo specify a target

$(PART1_EXEC): $(PART1_OBJECTS)
	$(CC) $(PART1_OBJECTS) -o $(PART1_EXEC) -lgmp

run_part1: $(PART1_SOURCES) $(PART1_EXEC)
	$(RUN) $(HOSTS) -n $(n) $(PART1_EXEC) 100

.c.o:
	$(CC) $(INCLUDE) -DDEBUG=$(DEBUG) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ *.*\# $(PART1_EXEC) core*
