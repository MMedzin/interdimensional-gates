#include "queues.h"


void add_to_queue(queue_elem** first, int rank, int source){
    queue_elem* new_elem = (queue_elem*) malloc(sizeof(queue_elem));
    new_elem->rank = rank;
    new_elem->source = source;
    new_elem->pending = TRUE;
    if(*first == NULL || (*first)->rank > rank){
        new_elem->next = *first;
        *first = new_elem;
        return;
    }
    else if((*first)->next == NULL){
        new_elem->next = NULL;
        (*first)->next = new_elem;
        return;
    }
    queue_elem** current_elem = &((*first)->next);
    while((*current_elem)->next != NULL){
        if((*current_elem)->rank > rank){
            new_elem->next = *current_elem;
            *current_elem = new_elem;
            return;
        }
        current_elem = &((*current_elem)->next);
    }
    if((*current_elem)->rank > rank){
        new_elem->next = *current_elem;
        *current_elem = new_elem;
    }
    else{
        new_elem->next = NULL;
        (*current_elem)->next = new_elem;
    }
}

void release_for_source(queue_elem** first, int source){
    if((*first)->source == source && (*first)->pending){
        (*first)->pending = FALSE;
    }
    else{
        queue_elem** curr_elem = &((*first)->next);
        while(curr_elem != NULL){
            if((*curr_elem)->source == source && (*curr_elem)->pending){
                (*curr_elem)->pending = FALSE;
                break;
            }
            curr_elem = &((*curr_elem)->next);
        }
    }
}

int get_position_for_source(queue_elem* first, int source){
    int pos = 0;
    queue_elem* curr_elem = first;
    while(curr_elem != NULL){
        if(curr_elem->source == source && curr_elem->pending){
            return pos;
        }
        curr_elem = curr_elem->next;
        pos++;
    }
    return -1;
}

int is_medium_free(queue_elem* first, int source, int mediums_count){
    int pos = get_position_for_source(first, source);
    if(pos < 0) return FALSE;
    int curr_pos = 0;
    queue_elem* curr_elem = first;
    while(curr_pos < pos){
        if((curr_pos % mediums_count) == (pos % mediums_count) && curr_elem->pending)
            return FALSE;
        curr_elem = curr_elem->next;
        curr_pos ++;
    }
    return TRUE;
}

void check_delete(queue_elem** first, int amount){
    printf("check delete'uje\n");
    if((*first)->pending){
        printf("\n\npending\n\n");
        return;
    }
    queue_elem** curr_elem = &((*first)->next);
    printf("printuje kolejke\n");
    print_queue(*first);
    for(int i = 0; i < amount-1; i++){
        if (*curr_elem == NULL || (*curr_elem)->pending){
            printf("%d curr is null.\n", i);
            return;
        }
        curr_elem = &((*curr_elem)->next);
    }
    curr_elem = first;
    *first = (*first)->next;
    for(int i = 0; i < amount-1; i++){
        free(*curr_elem);
        curr_elem = first;
        *first = (*first)->next;
    }
}

void free_queue(queue_elem** first){
    if(first == NULL) return;
    free_queue(&((*first)->next));
    free(*first);
}

void print_elem(queue_elem* elem){
    printf("rank: %d; source %d; pending: %s\n", elem->rank, elem->source,
           elem->pending ? "TRUE":"FALSE");
}

void print_queue(queue_elem* first){
    if(first == NULL) return;
    int num = 0;
    printf("%d. ", ++num);
    print_elem(first);

    queue_elem* curr_elem = first->next;
    while (curr_elem != NULL)
    {
        printf("%d. ", ++num);
        print_elem(curr_elem);
        curr_elem = curr_elem->next;
    }
}
