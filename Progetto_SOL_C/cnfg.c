#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnfg.h"
#define DIM_STRING 256


// funzione di parsing dei parametri di configurazione
int readConfig(cnfg* conf, char* path) {
    
    // apro il file
    FILE* cnfgFP = fopen(path, "r");
    
    // controllo la corretta apertura
    if(cnfgFP == NULL) {
        perror("Errore apertura file di configurazione!\n");
        fflush(stderr);
        return -1;
    }
    
    // parsing
    for(size_t i = 0; i < 13; i++) {
        char par[DIM_STRING], name[DIM_STRING];
        fscanf(cnfgFP, "%s %s", par, name);

        switch(i) {
            // n casse
            case 0:
                if(strncmp("K", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->K = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // n clienti
            case 1:
                if(strncmp("C", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->C = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // n nuovi clienti
            case 2:
                if(strncmp("E", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->E = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
                break;
            // tempo massimo speso per acquistare i prodotti da parte dei clienti
            case 3:
                if(strncmp("T", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->T = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
                break;
            // tempo di servizio di un cassiere per singolo prodotto
            case 4:
                if(strncmp("L", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->L = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // massimo numero di prodotti acquistabili
            case 5:
                if(strncmp("P", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->P = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // intervallo di tempo in cui un cliente controlla la coda di una nuova cassa
            case 6:
                if(strncmp("S", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->S = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // soglia di chiusura cassa da parte del direttore
            case 7:
                if(strncmp("S1", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->S1 = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // soglia di apertura nuova cassa
            case 8:
                if(strncmp("S2", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->S2 = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // n casse aperte inizialmente
            case 9:
                if(strncmp("openCD", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->openCD = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // intervallo di tempo in cui un cassiere aggiorna il direttore sul numero dei clienti in coda
            case 10:
                if(strncmp("refreshD", name, strlen(name)) == 0 && atoi(par) >= 0)
                    conf->refreshD = atoi(par);
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                     return -1;
                }
            break;
            // nome file di log per i clienti
            case 11:
                if(strncmp("logname1", name, strlen(name)) == 0 && strncmp("statsClients.txt", par, strlen(par)) == 0) {
                    strncpy(conf->logname1, par, strlen(par));
                }
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);

                    return -1;
                }
            break;
            // nome file di log per i cassieri
            case 12:
                if(strncmp("logname2", name, strlen(name)) == 0 && strncmp("statsCashiers.txt", par, strlen(par)) == 0) {
                    strncpy(conf->logname2, par, strlen(par));
                }
                else {
                    fprintf(stderr, "Errore lettura parametri di configurazione: parametro #%zu!\n", i);
                    fflush(stderr);
                    return -1;
                }
            break;
        }
    }
    // chiudo il file
    fflush(cnfgFP);
    fclose(cnfgFP);
    return 0;
}
