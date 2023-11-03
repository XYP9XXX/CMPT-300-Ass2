#include "list.h"
#include <stddef.h>

Node nodeList[LIST_MAX_NUM_NODES];

List totalList[LIST_MAX_NUM_HEADS];

List *currentAvailableList = NULL;

Node *currentAvailableNode = NULL;

bool isFirst = true;

List *List_create() {
    if (currentAvailableList == NULL && !isFirst) {
        return NULL;
    }
    if (isFirst) {
        for (int i = 0; i < LIST_MAX_NUM_HEADS - 1; ++i) {
            totalList[i].nextAvailableList = &totalList[i + 1];
            currentAvailableList = &totalList[0];
        }
        for (int i = 0; i < LIST_MAX_NUM_NODES - 1; ++i) {
            nodeList[i].nextAvailableNode = &nodeList[i + 1];
            currentAvailableNode = &nodeList[0];
        }
        isFirst = false;
    }
    List *newList = currentAvailableList;
    newList->current = NULL;
    newList->head = NULL;
    newList->tail = NULL;
    newList->size = 0;
    newList->status = 2;
    currentAvailableList = currentAvailableList->nextAvailableList;
    return newList;
}

int List_count(List *pList) {
    if (pList == NULL) return 0;
    return pList->size;
}

void *List_first(List *pList) {
    if (pList->head == NULL || pList->size == 0 || pList == NULL) return NULL;
    pList->current = pList->head;
    pList->status = 2;
    return pList->current->item;
}

void *List_last(List *pList) {
    if (pList->tail == NULL || pList->size == 0 || pList == NULL) return NULL;
    pList->current = pList->tail;
    pList->status = 2;
    return pList->current->item;
}

void *List_next(List *pList) {
    if (pList->status == LIST_OOB_END || pList == NULL) {
        return NULL;
    } else if (pList->current != NULL && pList->current->next == NULL) {
        pList->status = LIST_OOB_END;
        pList->current = pList->current->next;
        return NULL;
    } else if (pList->current == NULL && pList->status == LIST_OOB_START) {
        pList->current = pList->head;
        pList->status = 2;
        return pList->current->item;
    } else {
        pList->current = pList->current->next;
        return pList->current->item;
    }
}

void *List_prev(List *pList) {
    if (pList->status == LIST_OOB_START || pList == NULL) {
        return NULL;
    } else if (pList->current != NULL && pList->current->prev == NULL) {
        pList->current = pList->current->prev;
        pList->status = LIST_OOB_START;
        return NULL;
    } else if (pList->status == LIST_OOB_END) {
        pList->current = pList->tail;
        pList->status = 2;
        return pList->current->item;
    } else {
        pList->current = pList->current->prev;
        return pList->current->item;
    }
}

void *List_curr(List *pList) {
    if (pList->current != NULL) {
        return pList->current->item;
    } else {
        return NULL;
    }
}

void append_node(List *pList, Node *connectNode, void *pItem, bool first) {
    connectNode->next = NULL;
    connectNode->item = pItem;
    if (first) {
        pList->tail = connectNode;
        pList->head = connectNode;

        connectNode->prev = NULL;
        pList->current = connectNode;
        pList->status = 2;
        pList->size++;
    } else {
        connectNode->prev = pList->tail;
        pList->tail->next = connectNode;
        pList->current = connectNode;
        pList->status = 2;
        pList->tail = connectNode;
        pList->size++;
    }
}


int List_append(List *pList, void *pItem) {
    /*in general cases, when a new node append the list from the tail,
     * it should first :
     * 1. set a new node from nodeList, set the next node to null, set prev node to tail,
     *    set isHead to null and set isTail to address reference of the tail, set item to pItem.
     * 2. set tail.next to current node and set tail.isTail to null.
     * 3. the tail should move to the current node
     * 4. the current should move to the current node
     * 4. add pList size with 1
     * 5. return 0
     * There is a several special cases should be considered:
     * 1. the current pList has no node, which its size is 0
     *    In this case, we need to first set the head and the tail to the same node address, after initialize the node,
     *    set isHead to address of the head, and set isTail to address of the tail.
     * For error handling:
     * 1. the current nodeList has no node available, then should return -1
     * 2. if the pList itself is a NULL pointer, then should return -1
     * 3. if the pItem itself is a NULL pointer, then should return -1
     */
    // error handling
    if (pList == NULL || pItem == NULL || currentAvailableNode == NULL) {
        return LIST_FAIL;
    }
    // special case
    if (pList->size == 0) {
        append_node(pList, currentAvailableNode, pItem, true);
    } else {
        append_node(pList, currentAvailableNode, pItem, false);
    }
    currentAvailableNode = currentAvailableNode->nextAvailableNode;
    return LIST_SUCCESS;
}

void prepend_node(List *pList, Node *connectNode, void *pItem, bool first) {
    // add node before the head, first initial the node
    connectNode->item = pItem;
    connectNode->prev = NULL;

    if (first) {
        // if the pList is empty, then just append to the head, and point tail, current to the node itself.
        pList->tail = connectNode;
        pList->head = connectNode;
        pList->current = connectNode;

        connectNode->next = NULL;
        pList->status = 2;
        pList->size++;
    } else {
        // if the pList is not empty, then add before the head, move the head to the current one, and point the current to the new node.
        pList->head->prev = connectNode;
        connectNode->next = pList->head;
        pList->current = connectNode;
        pList->head = connectNode;
        pList->status = 2;
        pList->size++;
    }
}

int List_prepend(List *pList, void *pItem) {
    /* in general when add a node to the list should have those following steps：
     *      1. initialize the new node from nodeList, set it's item to pItem, prev to NULL, set next to pList->head,
     *         set isHead to the address of the pList->head and set isTail to the address of the pList->tail.
     *      2. set pList->head->prev to the address new node
     *      3. set pList->head to the address of the new node
     *      4. set pList->current to the address of the new node
     *      5. increase the pList->size
     *      6. increase the nodeLeastAvailableIndex
     *  The special case need to be considered:
     *      1. the pList->size is 0:
     *          In this case, we need to first set the head and the tail to the same node address,
     *          after initialize the node, set isHead to address of the head, and set isTail to address of the tail.
     * For error handling:
     * 1. the current nodeList has no node available, then should return -1
     * 2. if the pList itself is a NULL pointer, then should return -1
     * 3. if the pItem itself is a NULL pointer, then should return -1
     */
    // error handling
    if (pList == NULL || pItem == NULL || currentAvailableNode == NULL) {
        return LIST_FAIL;
    }
    // special case
    if (pList->size == 0) {
        prepend_node(pList, currentAvailableNode, pItem, true);
    } else {
        prepend_node(pList, currentAvailableNode, pItem, false);
    }
    currentAvailableNode = currentAvailableNode->nextAvailableNode;
    return LIST_SUCCESS;
}

void freeNode(Node *node) {
    // remove the item from node and move the node back to pool for future use.

    node->prev = NULL;
    node->next = NULL;
    node->item = NULL;
    node->nextAvailableNode = currentAvailableNode;
    currentAvailableNode = node;
}

void *List_trim(List *pList) {
    /* This function is used to remove the last node from the list
     * The idea is to remove the last node from the pList, the current implementation is to directly remove the node from
     * the list, and sign the address to the availableNode[] array. The detailed description will be:
     * 1. remove all the item from the node,
     * 2. let the previous node disconnect with this node,
     * 3. move the tail node to the previous node.
     * 4. move the current node to the previous node.
     * 5. sign the node address to the availableNode[availableNodeIndex]
     * 6. increase availableNodeIndex and decrease pList->size
     * 7. return the last item
     *
     * Here are several special cases:
     * 1. The pList only have one node, then after remove the node, the List will be available, which its head, tail, current
     *    point will be NULL, and the size will be 0.
     * 2. when pList is empty, return NULL
     * The error cases will be :
     * 1. if pList is NULL, return NULL
     */
    if (pList == NULL || pList->size == 0) {
        return NULL;
    }
    if (pList->size == 1) {
        void *item = pList->head->item;
        freeNode(pList->head);
        pList->head = NULL;
        pList->tail = NULL;
        pList->size = 0;
        pList->current = NULL;
        return item;
    } else {
        void *item = pList->tail->item;
        pList->tail->prev->next = NULL;
        pList->current = pList->tail->prev;
        freeNode(pList->tail);
        pList->tail = pList->current;
        pList->size--;
        return item;
    }
}

void List_concat(List *pList1, List *pList2) {
    /* The list concat connect pList2 to pList1, which pList2's head will add to pList1's tail, and then free the pList2.
     * So the general case will be :
     * 1. connect the pList1->tail->next to pList2->head
     * 2. let pList2->head->prev = pList1->tail
     * 3. let pList1->tail = pList2->tail
     * 4. put pList2 to the nextAvailableList
     *
     * There are some special cases:
     * 1. when the pList1 is empty, then pList1 will fully extend the pList2, but the pList1->current is still set as NULL.
     * 2. when the pList2 is empty, then do nothing and return
     *
     * For the error cases:
     * 1. when the pList1 or pList2 is NULL, return
     *
     */
    if (pList1 == NULL || pList2 == NULL) {
        return;
    } else {
        if (pList2->size == 0) {
            return;
        } else if (pList1->size == 0) {
            pList1->head = pList2->head;
            pList2->head->prev = NULL;
            pList1->tail = pList2->tail;
            pList1->size = pList2->size;
            pList1->current = pList2->current;
        } else {
            pList1->tail->next = pList2->head;
            pList2->head->prev = pList1->tail;
            pList1->tail = pList2->tail;
            pList1->size += pList2->size;
            pList1->current->next = pList2->head;
        }
        pList2->tail = NULL;
        pList2->head = NULL;
        pList2->current = NULL;
        pList2->size = 0;
        pList2->nextAvailableList = currentAvailableList;
        currentAvailableList = pList2;
    }

}

int List_insert_after(List *pList, void *pItem) {
    /*
     * This function will insert the new node right after the current node, and sign the current point to the new node/
     * In general cases:
     * 1. check whether the availableNodeList is empty, if it's not, then initial node in availableNodeList, otherwise,
     *    initial node in nodeList.
     * 2. let new_node->next to the pList->current->next
     * 3. let new_node->prev to the pList->current
     * 4. let the pList->current->next->prev to the address of new node
     * 5. let the pList->current->next = address of new node
     * 6. let the pList->current = address of new node
     * There are several special cases:
     * 1. if the current node is NULL and pList->status == LIST_OOB_START:
     *    1.1 initial node
     *    1.2 sign the pList->current to header and continue insertion
     * 2. if the current node is NULL and pList->status == LIST_OOB_END;
     *    just use List_append();
     * 3. if the pList->size == 0, then just use List_append() or List_prepend();
     * The error case will be:
     * 1. pItem is NULL, then return -1
     * 2. pList is NULL, then return -1
     */
    if (pList == NULL || pItem == NULL || currentAvailableNode == NULL) {
        return LIST_FAIL;
    }
    if (pList->size == 0 || (pList->current == NULL && pList->status == LIST_OOB_END)) {
        return List_append(pList, pItem);
    }
    if (pList->current == NULL && pList->status == LIST_OOB_START) {
        return List_prepend(pList, pItem);
    }
    currentAvailableNode->item = pItem;
    currentAvailableNode->prev = pList->current;
    currentAvailableNode->next = pList->current->next;

    pList->current->next->prev = currentAvailableNode;
    pList->current->next = currentAvailableNode;
    pList->current = currentAvailableNode;
    currentAvailableNode = currentAvailableNode->nextAvailableNode;
    pList->size++;
    return LIST_SUCCESS;
}

int List_insert_before(List *pList, void *pItem) {
    //List_insert_before is same as List_insert_after but in the opposite way.
    if (pList == NULL || pItem == NULL || currentAvailableNode == NULL) {
        return LIST_FAIL;
    }
    if (pList->size == 0 || (pList->current == NULL && pList->status == LIST_OOB_START)) {
        return List_prepend(pList, pItem);
    }
    if (pList->current == NULL && pList->status == LIST_OOB_END) {
        return List_append(pList, pItem);
    }
    currentAvailableNode->item = pItem;
    currentAvailableNode->prev = pList->current->prev;
    currentAvailableNode->next = pList->current;
    pList->current->prev->next = currentAvailableNode;
    pList->current->prev = currentAvailableNode;
    pList->current = currentAvailableNode;
    currentAvailableNode = currentAvailableNode->nextAvailableNode;
    pList->size++;
    return LIST_SUCCESS;
}

void List_free(List *pList, FREE_FN pItemFreeFn) {
    // error checking
    if (pList == NULL) return;
    Node *iter = pList->tail;
    // special cases handling: pLint is empty;
    if (pList->size == 0) {
        currentAvailableList->nextAvailableList = pList;
        currentAvailableList = pList;
        return;
        //another special case: pList->size == 1
    } else if (pList->size == 1) {
        (*pItemFreeFn)(iter->item);
        iter->prev = NULL;
        iter->next = NULL;
        iter->nextAvailableNode = currentAvailableNode;
        currentAvailableNode = iter;
    } else {
        //general cases
        while (iter != NULL) {
            // delete item
            (*pItemFreeFn)(iter->item);
            // remove each node from current list
            iter->next = NULL;
            iter->nextAvailableNode = currentAvailableNode;
            currentAvailableNode = iter;
            iter = iter->prev;
        }
    }
    //delete pList
    pList->current = NULL;
    pList->head = NULL;
    pList->tail = NULL;
    pList->size = 0;
    pList->status = 2;
    currentAvailableList->nextAvailableList = pList;
    currentAvailableList = pList;
}

void *List_remove(List *pList) {
    /*
     * This function will remove the current item, and make next item to the current one. So in general cases:
     * 1. the pList->current->prev->next = pList->current->next and pList->current->next->prev = pList->current-> prev
     * 2. use temp node = pList->current,
     * 3. the pList->current = pList->current->next;
     * 4. remove temp node.
     * There are some special cases：
     * 1. the pList only have one size:
     *    Just remove the node;
     * 2. the current pointer is NULL and status is LIST_OOD_START or LIST_OOD_END, then don't change the list and return NULL;
     * 3. if the current pointer is equal to pList->tail, then directly call List_trim
     * 4. if the current pointer is equal to pList->head, then should let pList->head = pList->head and move to the next stage.
     * 5. if the pList is empty, then do nothing and return NULL.
     * There are some error cases:
     * 1. the pList is NULL, then directly return NULL and do nothing.
     */
    if (pList == NULL || (pList->current == NULL && pList->status == LIST_OOB_END) ||
        (pList->current == NULL && pList->status == LIST_OOB_START) || pList->size == 0) {
        return NULL;
    }
    void *p = pList->current->item;
    if (pList->size == 1) {
        freeNode(pList->current);
        pList->current = NULL;
        pList->head = NULL;
        pList->tail = NULL;
        pList->size = 0;
        pList->status = 2;
    } else if (pList->current == pList->head) {
        pList->head = pList->head->next;
        pList->head->prev = NULL;
        freeNode(pList->current);
        pList->current = pList->head;
    } else if (pList->current == pList->tail) {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
        freeNode(pList->current);
        pList->current = pList->tail;
    } else {
        Node *preNode = pList->current->prev;
        Node *nextNode = pList->current->next;
        preNode->next = nextNode;
        nextNode->prev = preNode;
        freeNode(pList->current);
        pList->current = nextNode;
    }
    return p;
}

void *List_search(List *pList, COMPARATOR_FN pComparator, void *pComparisonArg) {
    /*
     * list search will start at the current point, and move to the end of the list, and the match is determined by teh client
     * so the general case will be:
     * 1. start at the current point, and call pComparator(pList->current->item, pComparisonArg), if it returns true/ or1,
     *    then, just return the item.
     * 2. if no item is found, then just return NULL, and current point will leave at the end of the list, status will be LIST_OOB_END;
     *
     * There are several special cases:
     * 1. the pList->size == 0, then just return NULL;
     * 2. the pList->size == 1, then call pComparator with the current point and pComparisonArg. sign current point to the NULL and status as LIST_OOB_END
     * 3. the pComparisonArg is NULL, then just return NULL.
     *
     * The error cases will be:
     * 1. pList == NULL, then directly return NULL.
     */
    // error checking
    if (pList == NULL) {
        return NULL;
    }
    // special cases handling
    if (pList->size == 0 || pComparisonArg == NULL) {
        return NULL;
    }
    if (pList->size == 1) {
        if (pComparator(pList->current->item, pComparisonArg)) {
            return pList->current->item;
        } else {
            pList->current = NULL;
            pList->status = LIST_OOB_END;
            return NULL;
        }
    } else {
        // general cases
        while (pList->current != NULL) {
            if (pComparator(List_next(pList), pComparisonArg)) {
                return pList->current->item;
            }
        }
        pList->status = LIST_OOB_END;
        return NULL;
    }
}
