#include <stdlib.h>

#include "mw_api.h"
#include "debug.h"

struct LinkedList
{
	LinkedList * next;
	mw_work_t * data;
}

/* array must be null terminated */
LinkedList * listFromArray(mw_work_t ** array);
LinkedList * new_linkedlist_node();
void move_node_to_end(LinkedList *);
int list_length(LinkedList *)
