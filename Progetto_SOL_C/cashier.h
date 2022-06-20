#ifndef cashier_h
#define cashier_h

#include "client.h"
#include <stdbool.h>
#include <pthread.h>

// struttura dei cassieri nel processo supermercato
typedef struct cshr {
    pthread_t ID; // tid
    bool open; // aperta/chiusa
    int nServedClients; // numero di clienti serviti
    double timeOpened; // tempo in cui la cassa e' rimasta aperta
    int nClosure; // numero di chiusura
    double serviceTime; // tempo di servizio totale
    int index; // indice
    int nProducts; // n di prodotti totale
    double totServiceTime;
} cashier;

// funzione di inizializzazione cassieri
cashier* initializeCashiers(int k);
// funzione di inzializzazione mutex dei cassieri
pthread_mutex_t* initializeCashiersMTX(int);
// funzione di inizializzazione cond dei cassieri
pthread_cond_t* initializeCashiersCOND(int);

#endif /* cashier_h */
