#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// funzione che inzializza la struttura dati dei clienti
client* initializeClients(int c) {
    client* clients = malloc(sizeof(client) * c);
    for(int i = 0; i < c; i++) {
        clients[i].ID = (pthread_t) -1;
        clients[i].index = i;
        clients[i].hasPaid = false;
        clients[i].isOut = false;
        clients[i].isChanging = false;
        clients[i].posInQueue = -1;
        clients[i].cashiersIndex = -1;
        clients[i].nQueueChanges = 0;
        clients[i].nProducts = 0;
        clients[i].enterSM.tv_sec = 0;
        clients[i].enterSM.tv_nsec = 0;
        clients[i].enterQueue.tv_sec = 0;
        clients[i].enterQueue.tv_nsec = 0;
        clients[i].enterService.tv_sec = 0;
        clients[i].enterService.tv_nsec = 0;
        clients[i].exitSM.tv_sec = 0;
        clients[i].exitSM.tv_nsec = 0;
    }
    return clients;
    
}

// funzione di inizializzazione cond dei clienti
pthread_cond_t* initializeClientsCOND(int c) {
    pthread_cond_t* clientsCOND = malloc(sizeof(pthread_cond_t) * c);
    for(int i = 0; i < c; i++) {
        clientsCOND[i] = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    }
    return clientsCOND;
}
