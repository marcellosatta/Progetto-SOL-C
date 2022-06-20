#include "queue.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

// creazione nuova coda
list_t* new_list() {
    list_t* list = malloc(sizeof(list_t));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    
    return list;
}

// inserimento in testa
void head_insert(list_t* list, client* newClientInQueue) {
    list_elem_t* new = malloc(sizeof(list_elem_t));
    new->client = newClientInQueue;
    new->client->posInQueue = list->size + 1;
    new->next = list->head;
    list->head = new;
    if(list->size == 0)
        list->tail = new;
    
    (list->size)++;
}

// rimozione dalla testa
void head_remove(list_t* list) {
    if(list->size == 1) {
       
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
        (list->size)--;
    }
    else if(list->size > 1) {
        list_elem_t* tmp = list->head;
        list->head = list->head->next;
        free(tmp);
        (list->size)--;
    }
    
}

// rimozione dalla coda
void tail_remove(list_t* list) {
    if(list->size == 1) {
        
        list->tail->client->isOut = true;
        
        free(list->tail);
        list->head = NULL;
        list->tail = NULL;
        
        (list->size)--;
    }
    else if(list->size > 1) {
        
        list_elem_t* curr = list->head;
        list_elem_t* prev = NULL;
        while(curr->next != NULL) {
            prev = curr;
            curr = curr->next;
        }
        list->tail->client->isOut = true;
        prev->next = NULL;
        list->tail = prev;
        free(curr);
    
        (list->size)--;
    }
}

// rimozione per elemento
void key_remove(list_t* list, client* clientPaid) {
    list_elem_t* prev = NULL;
    list_elem_t* curr = list->head;
    
    while(curr != NULL && curr->client->ID != clientPaid->ID) {
        prev = curr;
        curr = curr->next;
    }
    
    if(prev == NULL) {
        head_remove(list);
    }
    else if(curr != NULL && curr->next == NULL) {
        tail_remove(list);
    }
    else if(curr != NULL) {
        prev->next = curr->next;
        free(curr);
        (list->size)--;
    }
}

// funzione che aggiorna la posizione dei clienti in coda
void refreshPos(list_t* list) {
    list_elem_t* curr = list->head;
    
    while(curr != NULL) {
        curr->client->posInQueue--;
        curr = curr->next;
    }
}

