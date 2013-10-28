#include <assert.h>
#include "linked_list.h"

LinkedList * new_linkedlist_node()
{
	LinkedList * new_node = malloc(sizeof(LinkedList));
	new_node->next = NULL;
	new_node->data = NULL;
	return new_node;
}

/* array must be null terminated */
LinkedList * listFromArray(mw_work_t ** array)
{
	if(*array == NULL) return NULL;
	
	LinkedList * head = new_linkedlist_node();
	mw_work_t ** array_iterator = array;
	head->data = *array_iterator;
	array_iterator++;
	LinkedList * current_node = head;
	while(*array_iterator != NULL)
	{
		LinkedList * next_node = new_linkedlist_node();
        assert(next_node->next == NULL);
		next_node->data = *array_iterator;
		array_iterator++;
		current_node->next = next_node;
		current_node = current_node->next;
        assert(current_node->next == NULL);
	}
	return head;
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

/*
 * Warning! This function messes up the structure
 * of the list. Which is okay, as long as you always
 * have a reference to a node ahead of the node being
 * moved to the end of the list.
 */
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
	end->next = node;
	node->next = NULL;
}
