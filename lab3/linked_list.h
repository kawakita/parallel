#include <stdlib.h>

#include "mw_api.h"
#include "debug.h"

typedef struct LinkedList
{
  struct LinkedList * next;
  int index;
}
LinkedList;

/* array must be null terminated */
LinkedList * listFromArray(mw_work_t ** array);
LinkedList * new_linkedlist_node();
void addNode(LinkedList * cur_node, int index);
void move_node_to_end(LinkedList *);
int list_length(LinkedList *);
