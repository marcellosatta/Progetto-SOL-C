#ifndef client_h
#define client_h

#include <pthread.h>
#include <stdbool.h>

// struttura che descrive un cliente
typedef struct clnt {
    pthread_t ID; // tid
    int index; // indice
    bool hasPaid; // il cliente ha pagato
    bool isOut; // il cliente e' uscito dal SM
    bool isChanging; // il cliente sta cambiando cassa
    int posInQueue; // posizione in coda
    int cashiersIndex; // indice del cassiere scelto
    int nQueueChanges; // numero di code cambiate
    int nProducts; // numero di prodotti acquistati
    struct timespec enterSM; // istante in cui il cliente entra nel supermercato
    struct timespec enterQueue; // istante in cui il cliente entra in coda
    struct timespec enterService; // istante i cui il cliente inizia ad essere servito dal cassiere
    struct timespec exitSM; // istante in cui il cliente esce dal supermercato
} client;

// funzione che inzializza la struttura dai dei clienti
client* initializeClients(int);
// funzione di inizializzazione cond dei clienti
pthread_cond_t* initializeClientsCOND(int);
#endif /* client_h */
