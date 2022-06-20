#ifndef cnfg_h
#define cnfg_h

#include <stdio.h>

// struct che conterra' i parametri di configurazione
typedef struct configuration {
    int K; // n casse
    int C; // n clienti iniziali
    int E; // n nuovi clienti
    int T; // tempo max di acquito da parte dei clienti
    int L; // tempo di servizio di un cassiere per singolo prodotto
    int P; // n max di prodotti che un cliente puo' acquistare
    int S; // intervallo di tempo in cui un cliente controlla la coda di una nuova cassa
    int S1; // soglia di chiusura cassa da parte del direttore
    int S2; // soglia di apertura nuova cassa
    int openCD; // n casse aperte inizialmente
    int refreshD; // intervallo di tempo in cui un cassiere aggiorna il direttore sul numero dei clienti in coda
    char logname1[17]; // nome file di log per i clienti
    char logname2[18]; // nome file di log per i cassieri
} cnfg;

// funzione di parsing
int readConfig(cnfg*, char*);

#endif /* cnfg_h */

