CC=mpicc
CFLAGS=-Wall
RUN=mpirun
HOSTS=--hostfile ../hosts
n=4
PART1_SOURCE=master.c\
             mw_api.c\
             slave.c
PART1_OBJECTS=$(PART1_SOURCE:.c=.o)
PART1_EXEC=part1

all:
	echo specify a target

$(PART1_EXEC): $(PART1_OBJECTS)
	$(CC) $(PART1_OBJECTS) -o $(PART1_EXEC)

run_part1: $(PART1_SOURCES) $(PART1_EXEC)
	$(RUN) $(HOSTS) -n $(n) $(PART1_EXEC) $(p)

.c.o:
	$(CC) $(CFLAGS) -c  $< -o $@

clean:
	rm -f *.o *~ *.*\# $(PART1_EXEC)
