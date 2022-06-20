#ifndef director_h
#define director_h

#include <stdio.h>


int updatemax(fd_set, int);
int directorsControl(long);


// struttura cassa
typedef struct CD {
        bool open; // aperta/chiusa
        int nClients; // n di clienti in coda
    } cashDesk;

// struttura supermercato
typedef struct SM{
        cashDesk* cashDesks; // array di cassieri
        int nOpenCashDesk; // numero di casse aperte
    } supermarket;

// funzione di inizializzazione struttura
supermarket* initializeSM(int, int);
#endif /* director_h */
