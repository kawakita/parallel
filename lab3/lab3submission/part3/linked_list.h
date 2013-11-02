#include <stdlib.h>

#include "map_reduce.h"
#include "debug.h"

typedef struct LinkedList
{
	struct LinkedList * next;
	map_work_t * data;
} LinkedList;

/* array must be null terminated */
LinkedList * listFromArray(map_work_t ** array);
LinkedList * new_linkedlist_node();
void move_node_to_end(LinkedList *);
int list_length(LinkedList *);
