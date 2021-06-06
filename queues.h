#ifndef QUEUES_H
#define QUEUES_H

//#include "main.h"
#include <stdlib.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

typedef struct queue_elem {
    int rank;
    int source;
    int pending;
    struct queue_elem* next;
} queue_elem;

void add_to_queue(queue_elem** first, int rank, int source);

void release_for_source(queue_elem** first, int source);

void check_delete(queue_elem** first, int amount);

int get_position_for_source(queue_elem* first, int source);

int is_medium_free(queue_elem* first, int source, int mediums_count);

void free_queue(queue_elem** first);

void print_queue(queue_elem* first);

void print_elem(queue_elem* elem);

#endif