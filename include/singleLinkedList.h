/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file single_linked_list.h
 * @brief A generic single linked list.
 */

#ifndef SINGLE_LINKED_LIST_H_
#define SINGLE_LINKED_LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Node
{
    void *data;
    size_t size;
    struct Node *next;
} Node;

typedef struct List
{
    Node *head;
    Node *tail;
    int size;
} List;

List newList();
void pushBack(List *list, void *data, size_t size);
Node *popFront(List *list);
Node *popBack(List *list);
Node *popAt(List *list, int index);
Node *getNth(List *list, int index);
void printList(List *list);
void clearNode(Node *node);
void deleteList(List *list);
List *copyList(List *list);
bool cmpCharList(List *list1, List *list2);
bool cmpStrList(List *list1, List *list2);
bool cmpStringList(List *list1, List *list2);

#endif /* SINGLE_LINKED_LIST_H_ */