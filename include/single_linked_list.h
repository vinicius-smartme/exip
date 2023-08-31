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

#include "procTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    void *data;
    size_t size;
    struct Node *next;
} Node;

typedef struct List {
    Node *head;
    Node *tail;
    int size;
} List;

List new_list();
void push_back(List *list, void *data, size_t size);
Node* pop_front(List *list);
Node* pop_at(List *list, int index);
Node* getNth(List *list, int index);
void print_list(List *list);
void clear_node(Node *node);
void delete_list(List *list);
List* copy_list(List* list);

#endif /* SINGLE_LINKED_LIST_H_ */