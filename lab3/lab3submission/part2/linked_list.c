#include <assert.h>
#include "linked_list.h"

LinkedList * new_linkedlist_node()
{
	LinkedList * new_node = malloc(sizeof(LinkedList));
	new_node->next = NULL;
	new_node->data = NULL;
        new_node->index = -1;
	return new_node;
}

/* array must be null terminated */
LinkedList * listFromArray(mw_work_t ** array)
{
	if(*array == NULL) return NULL;
	
	LinkedList * head = new_linkedlist_node();
	mw_work_t ** array_iterator = array;
        int count = 0;
	head->data = *array_iterator;
        head->index = count;
	array_iterator++;
        count++;
	LinkedList * current_node = head;
	while(*array_iterator != NULL)
	{
		LinkedList * next_node = new_linkedlist_node();
        assert(next_node->next == NULL);
		next_node->data = *array_iterator;
                next_node->index = count;
		array_iterator++;
                count++;
		current_node->next = next_node;
		current_node = current_node->next;
        assert(current_node->next == NULL);
	}
	return head;
}

void addNode(LinkedList * cur_node)
{
  assert (cur_node->next == NULL);
  LinkedList * new_node = new_linkedlist_node();
  assert (new_node->next == NULL);
  cur_node->next = new_node;
}

int list_length(LinkedList * node)
{
	if(node == NULL) return 0;
	int length = 1;
	while(node->next != NULL)
	{
		node = node->next;
		length++;
	}
	return length;
}



void move_node_to_end(LinkedList * node)
{
	if(node == NULL)
	{
		DEBUG_PRINT(("Failed to move node to end of list, node is NULL"));
        return;
	}
	LinkedList * end = node;
	while(end->next != NULL)
	{
		end = end->next;
	}
    LinkedList * new_end_node = new_linkedlist_node();
    new_end_node->data = node->data;
    new_end_node->index = node->index;
	end->next = new_end_node;
}
