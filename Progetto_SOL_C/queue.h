#ifndef queue_h
#define queue_h

#include <stdio.h>
#include "client.h"

// nodo della coda
typedef struct _list_element {
    client* client; // cliente
    struct _list_element* next; // il prossimo
} list_elem_t;

// struttura dati che definisce la coda dei clienti presenti per cassa
typedef struct {
    list_elem_t* head;
    list_elem_t* tail;
    int size;
} list_t;

// creazione nuova coda
list_t* new_list(void);
// inserimento in testa
void head_insert(list_t* list, client*);
// rimozione dalla testa
void head_remove(list_t* list);
// rimozione dalla coda
void tail_remove(list_t* list);
// rimozione per elemento
void key_remove(list_t* list, client* clientPaid);
// funzione che aggiorna la posizione dei clienti in coda
void refreshPos(list_t* list);

#endif /* queue_h */
