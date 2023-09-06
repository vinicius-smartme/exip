/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file single_linked_list.c
 * @brief Decode and encode utility functions.
 */

#include "singleLinkedList.h"
#include "grammarGenerator.h"

List newList()
{
    List list = {.head = NULL, .tail = NULL, .size = 0};
    return list;
}

void pushBack(List *list, void *data, size_t size)
{
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->data = calloc(1, size + 1);
    memcpy(node->data, data, size);
    node->size = size;
    node->next = NULL;

    if (list->tail == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
}

Node *popFront(List *list)
{
    if (list->head == NULL)
    {
        return NULL;
    }

    Node *node = list->head;
    list->head = list->head->next;

    if (list->head == NULL)
    {
        list->tail = NULL;
    }

    list->size--;
    return node;
}

Node *popBack(List *list)
{
    return popAt(list, list->size - 1);
}

Node *popAt(List *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        return NULL;
    }

    Node *node = NULL;

    if (index == 0)
    {
        return popFront(list);
    }

    Node *prev = list->head;
    for (int i = 0; i < index - 1; i++)
    {
        prev = prev->next;
    }

    node = prev->next;
    prev->next = node->next;
    if (prev->next == NULL)
    {
        list->tail = prev;
    }

    list->size--;
    return node;
}

Node *getNth(List *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        return NULL;
    }
    else
    {
        if (index == 0)
        {
            return list->head;
        }
        else if (index == list->size - 1)
        {
            return list->tail;
        }
        else
        {
            Node *currentNode = list->head;
            for (int i = 0; i < index; i++)
            {
                currentNode = currentNode->next;
            }
            return currentNode;
        }
    }
}

void printList(List *list)
{
    Node *node = list->head;
    printf("[");
    while (node != NULL)
    {
        printf("%.*s", (int)node->size, (char *)node->data);
        node = node->next;
        if (node != NULL)
        {
            printf(", ");
        }
    }
    printf("]\n");
}

void clearNode(Node *node)
{
    if (node != NULL)
        free(node->data);
    node->next = NULL;
    node->size = 0;
}

void deleteList(List *list)
{
    Node *node = list->head;
    while (node != NULL)
    {
        Node *next = node->next;
        free(node->data);
        free(node);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

List *copyList(List *list)
{
    List *new_list = malloc(sizeof(List));
    if (!new_list)
    {
        return NULL;
    }
    new_list->size = list->size;
    new_list->head = NULL;
    new_list->tail = NULL;
    Node *current = list->head;
    while (current != NULL)
    {
        Node *new_node = malloc(sizeof(Node));
        if (!new_node)
        {
            deleteList(new_list);
            return NULL;
        }
        new_node->size = current->size;
        new_node->data = malloc(current->size);
        if (!new_node->data)
        {
            free(new_node);
            deleteList(new_list);
            return NULL;
        }
        memcpy(new_node->data, current->data, current->size);
        new_node->next = NULL;
        if (new_list->head == NULL)
        {
            new_list->head = new_node;
            new_list->tail = new_node;
        }
        else
        {
            new_list->tail->next = new_node;
            new_list->tail = new_node;
        }
        current = current->next;
    }
    return new_list;
}