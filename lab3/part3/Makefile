CC=mpicc
CFLAGS=-g -Wall
INCLUDE=-I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
RUN=mpirun
HOSTS=--hostfile ../../hosts
n=10
SOURCE=linked_list.c\
       master.c\
       test_map_reduce.c\
       map_reduce.c\
       supervisor.c\
       slave.c
OBJECTS=$(SOURCE:.c=.o)
EXEC=map
DEBUG=1

all:
	echo specify a target

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) -lglib-2.0

run: $(SOURCES) $(EXEC)
	$(RUN) $(HOSTS) -n $(n) $(EXEC)

.c.o:
	$(CC) $(INCLUDE) -DDEBUG=$(DEBUG) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ *.*\# $(EXEC) core*
