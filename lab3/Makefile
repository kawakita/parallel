CC=mpicc
CFLAGS=-Wall -g
RUN=mpirun
HOSTS=--hostfile ../hosts
p=8
n=8
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
	$(CC) $(PART1_OBJECTS) -o $(PART1_EXEC)

run_part1: $(PART1_SOURCES) $(PART1_EXEC)
	$(RUN) $(HOSTS) -n $(n) $(PART1_EXEC) $(p)

.c.o:
	$(CC) -DDEBUG=$(DEBUG) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ *.*\# $(PART1_EXEC) core*
