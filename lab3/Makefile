
CC=mpicc
CFLAGS=-Wall
PART1_SOURCE=master.c\
            mw_api.c\
			slave.c
PART1_OBJECTS=$(PART1_SOURCE:.c=.o)
PART1_EXEC=run_part_1

part1: $(PART1_OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c  $< -o $@

clean:
	rm -f *.o $(PART1_EXEC)
