#include "cashier.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>


// funzione di inizializzazione struttura dati cassieri
cashier* initializeCashiers(int k) {
    cashier* cashiers = malloc(sizeof(cashier) * k);
    
    for(int i = 0; i < k; i++) {
        cashiers[i].open = false;
        cashiers[i].nClosure = 0;
        cashiers[i].timeOpened = 0;
        cashiers[i].nServedClients = 0;
        cashiers[i].ID = (pthread_t) -1;
        cashiers[i].index = i;
        unsigned int seed = (unsigned int) (time(NULL) + i);
        double st = (rand_r(&seed) % 60) + 20;
        cashiers[i].serviceTime = st/1000;
        cashiers[i].totServiceTime = 0;
    }
    return cashiers;
}

// funzione di inzializzazione mutex dei cassieri
pthread_mutex_t* initializeCashiersMTX(int k) {
    pthread_mutex_t* cashiersMTX = malloc(sizeof(pthread_mutex_t) * k);
    for(int i = 0; i < k; i++) {
        cashiersMTX[i] = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    }
    return cashiersMTX;
}

// funzione di inizializzazione cond dei cassieri
pthread_cond_t* initializeCashiersCOND(int k) {
    pthread_cond_t* cashiersCOND = malloc(sizeof(pthread_cond_t) * k);
    for(int i = 0; i < k; i++) {
        cashiersCOND[i] = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    }
    return cashiersCOND;
}
