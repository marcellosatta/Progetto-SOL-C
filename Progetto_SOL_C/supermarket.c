#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "supermarket.h"
#include "cnfg.h"
#include "cashier.h"
#include "client.h"
#include "queue.h"

#define N 256
#define SOCKNAME "socketSM_D"
#define UNIX_PATH_MAX 100

// variabili SIGQUIT e SIGHUP
static volatile sig_atomic_t sigquit=0;
static volatile sig_atomic_t sighup=0;

// handler SIGQUIT
static void SIGQUIThandler(int x) {
    sigquit=1;
}
//handler SIGHUP
static void SIGHUPhandler(int x) {
    sighup=1;
}

// puntatore a file di log per cassieri
static FILE* statsCashiers;
// puntatore a file di log per clienti
static FILE* statsClients;


// numero di casse aperte
static int nOpenCashiers = 0;
// numero totale di clienti all'interno del supermercato
static int totalClients = 0;
// clienti usciti dal supermercato
static int exitedClients = 0;

// mutex variabile totalClients
static pthread_mutex_t  totalCLientsMTX = PTHREAD_MUTEX_INITIALIZER;
// mutex variabile numero di casse aperte
static pthread_mutex_t  nOpenCashDeskMTX = PTHREAD_MUTEX_INITIALIZER;
// mutex di controllo per l'entrata di nuovi clienti
static pthread_mutex_t newClientsMTX = PTHREAD_MUTEX_INITIALIZER;

// mutex scelta cassa
static pthread_mutex_t chooseQueueMTX = PTHREAD_MUTEX_INITIALIZER;
// mutex apertura e chiusura casse
static pthread_mutex_t OpenCloseCashDeskMTX = PTHREAD_MUTEX_INITIALIZER;
// array di mutex di entrata in coda da parte dei clienti
static pthread_mutex_t* enterQueueMTX;
// array di mutex di inzio servizio del cassiere per cliente
static pthread_mutex_t* enterServiceMTX;

// array condition variable di inizio servizio del cassiere
static pthread_cond_t* cashiersServiceCOND;
// array condition variable che avvisa il cliente quando e' in prima posizione
static pthread_cond_t* firstPosClientCOND;
// array condition variable che avvisa il cassiere quando il cliente ha pagato
static pthread_cond_t* clientHasPaidCOND;
// array condition variable che avvisa il cliente che puo' uscire dal SM
static pthread_cond_t* clientIsOutCOND;

// condition variable di entrata nuovi clienti
static pthread_cond_t  newClientsCOND = PTHREAD_COND_INITIALIZER;

// attributo di creazione dei thread cassieri (resi detached)
static pthread_attr_t attr;

// struttura che conterra' i parametri di configurazione
static cnfg config;
// struttura che conterra' le informazioni relative ai cassieri
static cashier* cashiers;
// struttura che conterra' le informazioni relative ai clienti
static client* clients;
// struttura che descrivera' le code alle varie casse
static list_t** queues;

int main(int argc, char* argv[]) {
    
    // lettura parametri di configurazione
    if(readConfig(&(config), argv[1]) != 0) {
        perror("Errore durante la lettura del file di configurazione.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    
    // aprtura file di log per i clienti
    if ((statsClients = fopen(config.logname1, "w")) == NULL) {
      fprintf(stderr, "Stats file opening failed1");
      exit(EXIT_FAILURE);
      }
    
    // apertura file di log per i cassieri
    if ((statsCashiers = fopen(config.logname2, "w")) == NULL) {
         fprintf(stderr, "Stats file opening failed2");
         exit(EXIT_FAILURE);
         }

    // installazione handler SIGQUIT
    struct sigaction quit;
    memset (&quit, 0, sizeof(quit));
    quit.sa_handler = SIGQUIThandler;
    sigaction(SIGQUIT,  &quit, NULL);
    
    // installazione handler SIGHUP
    struct sigaction hup;
    memset (&hup, 0, sizeof(hup));
    hup.sa_handler = SIGHUPhandler;
    sigaction(SIGHUP,  &hup, NULL);
    
    // maschero la sigpipe per evitare effetti indesiderati durante la comunicazione su socket
    signal(SIGPIPE, SIG_IGN);

    // inizializzazione mutex di servizio per i cassieri
    enterServiceMTX = initializeCashiersMTX(config.K);
    enterQueueMTX = initializeCashiersMTX(config.K);

    // inizializzazione condition variable per le operazioni di servizio del cassiere
    cashiersServiceCOND = initializeCashiersCOND(config.K);
    firstPosClientCOND = initializeClientsCOND(config.C);
    clientHasPaidCOND = initializeCashiersCOND(config.K);
    clientIsOutCOND = initializeClientsCOND(config.C);
    
    // inzializzazione struttura dati dei cassieri
    cashiers = initializeCashiers(config.K);
    
    // creazione code per le varie casse
    queues = malloc(sizeof(list_t) * config.K);
    
    // init attributo detached per i cassieri
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    // controllo struttura cassieri
    if(cashiers == NULL) {
        perror("Errore durante l'inizializzazione delle casse.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    // creazione thread cassieri (detached)
    for(long i = 0; i < config.openCD; i++) {
        if(pthread_create(&(cashiers[i].ID), &attr, (void*)&openCashiersDesk, (void*) i) != 0) {
            perror("Errore durante la creazione del thread cassiere.\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
    }
    
    // inizializzazione struttura dati clienti
    clients = initializeClients(config.C);
    
    // creazione thread clienti
    for(long i = 0; i < config.C; i++) {
        if(pthread_create((void*)&clients[i].ID, &attr, (void*)&clientEnterSM, (void*)i) != 0) {
            perror("Errore durante la creazione del thread cliente.\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        // incremento numero di clienti presenti all'interno del SM
        pthread_mutex_lock(&totalCLientsMTX);
        totalClients++;
        pthread_mutex_unlock(&totalCLientsMTX);
    }
   
    
    // ciclo che permette, quando escono E clienti di farne entrare altri E
    while(!sighup && !sigquit) {
        // se non ne sono ancora usciti E, aspetta...
        pthread_mutex_lock(&newClientsMTX);
        if(exitedClients < config.E) {
            pthread_cond_wait(&newClientsCOND, &newClientsMTX);
        }
        pthread_mutex_unlock(&newClientsMTX);
        // controllo segnali (inserito dopo la wait)
        if(sighup || sigquit) break;

        int j = 0;
        // il cliente che entra "sostituisce" quello che esce all'interno dell'array
        for(long i = 0; i < config.C && !sigquit; i++) {
            if(clients[i].index == -1 && j < config.E && !sigquit) {
                // reset
                clients[i].ID = (pthread_t) -1;
                clients[i].index = (int) i;
                clients[i].isOut = false;
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
                // se non viene chiamata la sigquit
                if(!sigquit) {
                    // creazione nuovo thread cliente
                    if(pthread_create((void*)&clients[i].ID, &attr, (void*)&clientEnterSM, (void*)i) != 0) {
                        perror("Errore durante la creazione del thread cliente.\n");
                        fflush(stderr);
                        exit(EXIT_FAILURE);
                    }
                    // incremento numero di clienti presenti all'interno del SM
                pthread_mutex_lock(&totalCLientsMTX);
                totalClients++;
                pthread_mutex_unlock(&totalCLientsMTX);
                j++;
                }
            }
        }
        // reset numero di clienti usciti
        exitedClients = 0;
        j = 0;
    }
    // esco dal while se arriva o il segnale SIGQUIT o il segnale SIGHUP
  
    // se arriva il SIGQUIT non esco finche' tutti i cassieri sono chiusi
    if(sigquit) {
        while(nOpenCashiers != 0) {
            for(int i = 0; i < config.K; i++) {
                if(cashiers[i].open == true) {
                    pthread_cond_signal(&cashiersServiceCOND[i]);
                    pthread_cond_signal(&clientHasPaidCOND[i]);
                }
            }
        }
    }
    
    // se arriva la SIGHUP non esco finche' tutti i cassieri non sono chiusi
    if(sighup) {
         while(nOpenCashiers != 0) {
            for(int i = 0; i < config.K; i++) {
                if(cashiers[i].open == true) {
                    pthread_cond_signal(&cashiersServiceCOND[i]);
                    pthread_cond_signal(&clientHasPaidCOND[i]);
                }
            }
        }
    }
    
    // CLEAN-UP:
    
    // distruzione mutex e condition variables
    pthread_mutex_destroy(&totalCLientsMTX);
    pthread_mutex_destroy(&nOpenCashDeskMTX);
    pthread_mutex_destroy(&newClientsMTX);
    pthread_mutex_destroy(&OpenCloseCashDeskMTX);

    pthread_cond_destroy(&newClientsCOND);

    for(int i = 0; i < config.K; i++) {
        pthread_mutex_destroy(&enterQueueMTX[i]);
        pthread_mutex_destroy(&enterServiceMTX[i]);
    }
    for(int i = 0; i < config.K; i++) {
        pthread_cond_destroy(&cashiersServiceCOND[i]);
        pthread_cond_destroy(&clientHasPaidCOND[i]);
    }
    for(int i = 0; i < config.C; i++) {
        pthread_cond_destroy(&firstPosClientCOND[i]);
        pthread_cond_destroy(&clientIsOutCOND[i]);
    }
  
    // distruzione attributo pthread_create
    pthread_attr_destroy(&attr);
    
    
    // comunico al direttore, tramite socket che il SM sta chiudendo
    char buf[N];
    int SMFD;

    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    strncpy(address.sun_path, SOCKNAME, UNIX_PATH_MAX);
    address.sun_family=AF_UNIX;
    
    if((SMFD = socket(AF_UNIX,SOCK_STREAM,0)) == -1) {
        perror("Errore durante la socket()!\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
        
    while(connect(SMFD, (struct sockaddr*) &address, sizeof(address)) == -1) {
        if (errno == ENOENT)
            sleep(1);
        else exit(EXIT_FAILURE);
    }
    
    memset(&buf, 0, sizeof( char) * N);
    sprintf(buf, "close");
    long n = write(SMFD,buf,strlen(buf));
    if (n < 0) {
        perror("ERROR writing from socket");
        fflush(stderr);
    }

    memset(&buf, 0, sizeof(char) * N);
    n = read(SMFD,buf,N);
    if (n < 0) {
        perror("ERROR reading from socket");
        fflush(stderr);
    }

    // se ricevo l'ok chiudo i file di stats ed esco
    if(strncmp("ok", buf, strlen(buf)) == 0) {
        unlink(SOCKNAME);
        fclose(statsCashiers);
        fclose(statsClients);
        return 0;
    }
}


// funzione di controllo thread cassiere
void* openCashiersDesk(long i) {
    // timespec per controllare il tempo di aggiornamento del direttore da parte del singolo
    // cassiere in merito al numero di clienti presenti in coda
    struct timespec start, finish;
    double elapsed, elapsed1 = 0;

    // timespec per calcolare il tempo di lavoro del cassiere
    struct timespec startWork, finishWork;

    // mi metto in comunicazione con il direttore
    char buf[N];
    memset(&buf, 0, sizeof(char) * N);
    
    long n;
    
    // connetto il socket per comunicare con il direttore
    int sockFD;
    
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    strncpy(address.sun_path, SOCKNAME, UNIX_PATH_MAX);
    address.sun_family=AF_UNIX;
    
    if((sockFD = socket(AF_UNIX,SOCK_STREAM,0)) == -1) {
        perror("Errore durante la socket()!\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    
    while(connect(sockFD, (struct sockaddr*) &address, sizeof(address)) == -1) {
        if (errno == ENOENT)
            sleep(1);
        else exit(EXIT_FAILURE);
    }
    
    
    // apro la cassa
    pthread_mutex_lock(&OpenCloseCashDeskMTX);
    cashiers[i].open = true;
    pthread_mutex_unlock(&OpenCloseCashDeskMTX);
    
    // incremento il numero di cassieri aperti
    pthread_mutex_lock(&nOpenCashDeskMTX);
    nOpenCashiers++;
    pthread_mutex_unlock(&nOpenCashDeskMTX);
    
    // flag che mi indica se è stata chiamata la SIGHUP
    bool hup = false;
    
    // creo la coda
    queues[i] = new_list();
    
    // indice con cui indico il cliente che sta venendo servito durante il ciclo del successivo while
    int j = 0;
    
    // il cassiere inizia il turno di lavoro...
    clock_gettime(CLOCK_MONOTONIC, &startWork);

    while(!sigquit) {
        // clock con cui tengo traccia dell'intervallo di tempo in cui un cassiere comunica al direttore il numero di
        // clienti presenti in cassa
        clock_gettime(CLOCK_MONOTONIC, &start);

        pthread_mutex_lock(&enterServiceMTX[i]);
        // se la coda è vuota...
        if(queues[i]->size == 0) {
            // ... aspetto che ci sia almeno un cliente in coda
            pthread_cond_wait(&cashiersServiceCOND[i], &enterServiceMTX[i]);
        }
        // mi salvol'indice del cliente da servire attualmente (all'interno della struttura clients)
        j = queues[i]->tail->client->index;
        
        // mando il segnale al cliente che è in prima posizione
        pthread_cond_signal(&firstPosClientCOND[j]);
        
        // tempo di servizio tramite nanosleep
        struct timespec serviceTime, serviceTimeRemain;
        double time = cashiers[i].serviceTime + queues[i]->tail->client->nProducts * (config.L / 1000.0);
        serviceTime.tv_sec = time;
        serviceTime.tv_nsec = (time - serviceTime.tv_sec) * 1000000000;
        clients[j].enterService = serviceTime;
        
        nanosleep(&serviceTime, &serviceTimeRemain);
        
        // aggiungo il tempo di servizio totale speso da parte del cassiere
        double tmp = (serviceTime.tv_sec);
        tmp += serviceTime.tv_nsec / 1000000000.0;
        cashiers[i].totServiceTime += tmp;
        // aggiungo il numero di prodotti elaborati
        cashiers[i].nProducts += clients[j].nProducts;

        // aggiorno le informazioni del cliente
        clients[j].isChanging = false;
        // il cliente è uscito
        clients[j].isOut = true;
        // decremento la sua posizione in coda
        clients[j].posInQueue--;
        
        
        // lo rimuovo dalla coda
        tail_remove(queues[i]);
        // aggiorno la posizione di tutti i clienti che sono presenti in coda
        refreshPos(queues[i]);
        
        // se il cliente non ha ancora pagato aspetto
        if(clients[j].hasPaid == false) {
            pthread_cond_wait(&clientHasPaidCOND[i], &enterServiceMTX[i]);
        }
        // altrimenti mando il segnale di uscita
        pthread_cond_signal(&clientIsOutCOND[j]);
        pthread_mutex_unlock(&enterServiceMTX[i]);
        

        
        // prendo il tempo del servizo del cassiere attuale e se esso è >= dell'intervallo di tempo
        // in cui il cassiere deve aggiornare il direttore sul numero dei clienti in coda scrivo sul socket a meno di SIGQUIT e SIGHUP
        clock_gettime(CLOCK_MONOTONIC, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
     
        if(elapsed + elapsed1 >= (config.refreshD / 1000.0) && !sigquit && !sighup) {
            memset(&buf, 0, sizeof(char) * N);
            sprintf(buf, "cashier_%ld_%d", i, queues[i]->size);
            // scrivo sul socket al direttore l'indice del cassiere e il numero di clienti presenti in coda
            
            n = write(sockFD,buf,strlen(buf));
            if (n < 0) {
                perror("ERROR writing to socket");
                fflush(stderr);
            }
            memset(&buf, 0, sizeof(char) * N);
            // aspetto la risposta del direttore
            n = read(sockFD,buf,N);
            if (n < 0) {
                perror("ERROR writing to socket");
                fflush(stderr);
            }
            
            // se il direttore ha decido di aprire o chiudere qualche cassa:
            if(strncmp(buf, "null", strlen(buf)) != 0 && !sigquit) {
                
                // faccio il parsing tramite tokenizzazione della stringa scritta sul socket dal direttore
                char* token;
                char* rest = buf;
                bool toClose = false;
                bool toOpen = false;
                long cashierIndex = -1;
                int k = 0;
                
                while ((token = strtok_r(rest, "_", &rest)) && k < 2) {
                    if(strncmp("close", token, strlen(token)) == 0) {
                        toClose = true;
                    }
                    else if (strncmp("open", token, strlen(token)) == 0) {
                        toOpen = true;
                    }
                    else if(toClose == true) {
                        cashierIndex = atoi(token);
                    }
                    else if (toOpen == true) {
                        cashierIndex = atoi(token);
                    }
                    k++;
                }
                
                // se il direttore ha deciso di chiudere la cassa esco dal while tramite break
                if(toClose == true && 0 <= cashierIndex && cashierIndex < config.K && !sigquit) {
                    if(cashiers[cashierIndex].open == true && cashierIndex == i && !sigquit) {
                        if(nOpenCashiers > 1 && totalClients > 0)
                            break;
                    }
                }
                // se il direttore ha deciso di aprire una nuova cassa creo un nuovo thread cassiere
                else if (toOpen == true && 0 <= cashierIndex && cashierIndex < config.K && !sigquit) {
                    if(cashiers[cashierIndex].open == false && !sigquit) {
                        if(pthread_create(&(cashiers[cashierIndex].ID), &attr, (void*)&openCashiersDesk, (void*) cashierIndex) != 0) {
                            perror("Errore durante lla creazione del thread cassiere.\n");
                            fflush(stderr);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            elapsed1 = 0;
        }
        else {
            elapsed1 += elapsed;
        }
           
        // incremento il numero totale di clienti serviti
        cashiers[i].nServedClients++;
        
        // flag che mi avverte se il cassire deve continuare a far scorrere i clienti
        if(sighup) {
            hup = true;
            // se è stata chiamata la SIGHUP e non ci sono più clienti da servire
            // (la coda è vuota) esco dal while emi appresto a chiudere la cassa
            if(queues[i]->size == 0) {
                break;
            }
        }
        // se è stata chiamata la SIGQUIT esco dal ciclo e mi appresto a chiudere la cassa
        if(sigquit) {
            break;
        }
    }
    // decremento il numero di cassieri aperti
    pthread_mutex_lock(&nOpenCashDeskMTX);
    nOpenCashiers--;
    pthread_mutex_unlock(&nOpenCashDeskMTX);
    
    // chiudo la cassa
    pthread_mutex_lock(&OpenCloseCashDeskMTX);
    cashiers[i].open = false;
    pthread_mutex_unlock(&OpenCloseCashDeskMTX);

    // se non è stato segnalato SIGHUP oppure è stato segnalato ma questo non è l'ultimo cassiere aperto
    // (ovvero se un cassiere sta chiudendo perchè deciso dal direttore)
    if(!hup || (sighup && nOpenCashiers > 1)) {
        while(queues[i]->size != 0) {
            // i clienti vengono fatti scorrere e successivamente assegnati ad altri cassieri
            // finchè non c'è più nessuno in coda
            pthread_mutex_lock(&enterServiceMTX[i]);
            // il cliente deve cambiare cassa
            clients[j].isChanging = true;

            if(queues[i]->size == 0) {
                pthread_cond_wait(&cashiersServiceCOND[i], &enterServiceMTX[i]);
            }
            int j = queues[i]->tail->client->index;
            pthread_cond_signal(&firstPosClientCOND[j]);
            
            clients[j].isOut = true;

            refreshPos(queues[i]);
            tail_remove(queues[i]);
            
            if(clients[j].hasPaid == false) {
                pthread_cond_wait(&clientHasPaidCOND[i], &enterServiceMTX[i]);
            }
            pthread_cond_signal(&clientIsOutCOND[j]);
            pthread_mutex_unlock(&enterServiceMTX[i]);
        }
    }
    
    // la cassa chiude, incremento il numero di chiusure
    cashiers[i].nClosure++;
    // prendo il tempo di attività e lo aggiungo a quello totale di apertura
    clock_gettime(CLOCK_MONOTONIC, &finishWork);
    double totTime = (finishWork.tv_sec - startWork.tv_sec);
    totTime += (finishWork.tv_nsec - startWork.tv_nsec) / 1000000000.0;
    cashiers[i].timeOpened += totTime;
    
    // scrivo su file i dati di lavoro del cassiere ogni volta che chiude la cassa
    fprintf(statsCashiers, "Sono il cassiere %ld, ho elaborato %d prodotti acquistati da %d clienti. Ho lavorato per %.3f s con un tempo di servizio medio di %.3f s. Ho chiuso la cassa %d volte\n",
        (long) cashiers[i].index,
        cashiers[i].nProducts,
        cashiers[i].nServedClients,
        cashiers[i].timeOpened,
        cashiers[i].totServiceTime / (double) cashiers[i].nServedClients,
        cashiers[i].nClosure);
    return NULL;
}


// funzione di controllo del thread cliente
void* clientEnterSM(long i) {
    
    // prendo il tempo di entrata all'interno del SM
    clock_gettime(CLOCK_MONOTONIC, &clients[i].enterSM);
    // calcolo il seed per i dati pseudo-casuali
    unsigned long long int seed = (unsigned int) clients[i].ID * time(NULL) + i;
    // calcolo il numero di prodotti acquistati
    clients[i].nProducts = rand_r((unsigned int*)&seed) % config.P;
    
    // se il numero di prodotti acquistati è 0 il cliente esce chiedendo prima il permesso al direttore
    if(clients[i].nProducts == 0) {
        long index = i;
        char buf[N];
        long n;
        int sockFD;
        struct sockaddr_un address;
        memset(&address, 0, sizeof(struct sockaddr_un));
        strncpy(address.sun_path, SOCKNAME, UNIX_PATH_MAX);
        address.sun_family=AF_UNIX;
        
        if((sockFD = socket(AF_UNIX,SOCK_STREAM,0)) == -1) {
            perror("Errore durante la socket()!\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        while(connect(sockFD, (struct sockaddr*) &address, sizeof(address)) == -1) {
            if (errno == ENOENT)
                sleep(1);
            else exit(EXIT_FAILURE);
        }
        
        // scrivo 0
        memset(&buf, 0, sizeof(char) * N);
        sprintf(buf, "0");
        n = write(sockFD,buf,strlen(buf));
        if (n < 0) {
            perror("ERROR writing to socket");
            fflush(stderr);

        }
        memset(&buf, 0, sizeof(char) * N);
        n = read(sockFD,buf,N);
        if (n < 0) {
            perror("ERROR writing to socket");
            fflush(stderr);

        }
        // se ricevo 1 il cliente può uscire
        if(atoi(buf) == 1) {
            // scrivo -1 sull'index del client per differenziarlo da coloro ancora all'interno del SM
            clients[i].index = -1;
            // incremento il numero di clienti usciti dal SM
            exitedClients++;
            // se sono usciti E clienti mando il segnale per farne entrare altri E
            pthread_mutex_lock(&newClientsMTX);
            if(exitedClients == config.E) {
                pthread_cond_signal(&newClientsCOND);
            }
            pthread_mutex_unlock(&newClientsMTX);
            
            // decremento il numero di clienti presenti all'interno del SM
            pthread_mutex_lock(&totalCLientsMTX);
            totalClients--;
            pthread_mutex_unlock(&totalCLientsMTX);
            
            // prendo il tempo di uscita
            clock_gettime(CLOCK_MONOTONIC, &clients[i].exitSM);
            double totTime = (clients[i].exitSM.tv_sec - clients[i].enterSM.tv_sec);
            totTime += (clients[i].exitSM.tv_nsec - clients[i].enterSM.tv_nsec) / 1000000000.0;
            
            // scrivo sul file di log
            fprintf(statsClients, "Sono il cliente %ld, ho comprato %d oggetti e ho impiegato %.3f s, di cui - per comprare i prodotti, - in coda e - tempo di servizio. Cassa assegnata numero: -, code cambiate: -\n",
                index,
                clients[i].nProducts,
                totTime);
            // esco
            return NULL;
        }
    }

    // se il cliente ha comprato almeno un prodotto
    
    // calcolo il tempo in cui ha scelto i prodotti da acquistare
    int buyingTime = (rand_r((unsigned int*)&seed) % ((int) config.T - 10)) +10;
    struct timespec buyingTimespec, buyingTimespec1;
    buyingTimespec.tv_sec = 0;
    buyingTimespec.tv_nsec = ((buyingTime / 1000.0) * 1000000000);
    // lo assegno all'interno della struttura del cliente
    clients[i].enterQueue.tv_sec = buyingTimespec.tv_sec;
    clients[i].enterQueue.tv_nsec = buyingTimespec.tv_nsec;
    // nanosleep tempo speso per la scelta dei prodotti da acquistare
    nanosleep(&buyingTimespec, &buyingTimespec1);
       
    // scelta prima cassa (random)
    pthread_mutex_lock(&chooseQueueMTX);
    int firstQueue = -1;
       do {
           firstQueue = rand_r((unsigned int*)&seed) % config.K;
       }while(cashiers[firstQueue].open != true && !sigquit);
    clients[i].cashiersIndex = firstQueue;
    pthread_mutex_unlock(&chooseQueueMTX);
    
    // aggiunta alla coda
    pthread_mutex_lock(&enterQueueMTX[clients[i].cashiersIndex]);
    head_insert(queues[clients[i].cashiersIndex], &clients[i]);
    pthread_mutex_unlock(&enterQueueMTX[clients[i].cashiersIndex]);

    // varaibili per il calcolo di una cassa migliore
    int min = clients[i].posInQueue;
    int pos = clients[i].cashiersIndex;
    int checkedQueue = 0;
    int j = -1;
    if(sigquit) {
        pthread_mutex_lock(&totalCLientsMTX);
        totalClients--;
        pthread_mutex_unlock(&totalCLientsMTX);
        return NULL;
    }
    // finchè il cliente non è uscito o finchè non viene inviato il SIGQUIT
    while(clients[i].isOut == false) {
        // intervallo di tempo in cui un cliente controlla se è vantaggioso
        // spostarsi in un'altra cassa
        struct timespec interval, intervalRemain;
        interval.tv_sec = config.S / 1000;
        interval.tv_nsec = config.S / 1000.0 * 1000000000.0;
        
        while(clients[i].posInQueue > config.S2) {
            // nanosleep S millisecondi
            nanosleep(&interval, &intervalRemain);
            
            // scelta cassa più vanatggiosa
            pthread_mutex_lock(&chooseQueueMTX);
            for(int i = 0; i < config.K; i++) {
                if(cashiers[i].open == true) {
                    if(min > queues[i]->size + 1) {
                        min = queues[i]->size + 1;
                        pos = i;
                        checkedQueue++;
                    }
                }
            }
                
            pthread_mutex_unlock(&chooseQueueMTX);
            
            // se la cassa scelta è diversa dall'iniziale
            if(firstQueue != pos) {
                // rimuovo il cliente dalla vecchia coda
                pthread_mutex_lock(&enterQueueMTX[firstQueue]);
                key_remove(queues[firstQueue], &clients[i]);
                pthread_mutex_unlock(&enterQueueMTX[firstQueue]);
                
                // cambio il cassiere scelto
                clients[i].cashiersIndex = pos;
                // incremento il numero di casse cambiate
                clients[i].nQueueChanges++;
                
                // aggiungo il cliente nella nuova cassa
                pthread_mutex_lock(&enterQueueMTX[pos]);
                head_insert(queues[pos], &clients[i]);
                pthread_mutex_unlock(&enterQueueMTX[pos]);
                firstQueue = pos;
            }
            // se è stato segnalato SIGQUIT o se un nuovo cliente è entrato durante la chiusura del SM
            // decremento il numero dei clienti presenti all'interno del SM ed esco
            if(sigquit){
                pthread_mutex_lock(&totalCLientsMTX);
                totalClients--;
                pthread_mutex_unlock(&totalCLientsMTX);
                return NULL;
            }
        }
        // se è stato segnalato SIGQUIT o se un nuovo cliente è entrato durante la chiusura del SM
        // decremento il numero dei clienti presenti all'interno del SM ed esco
        if(sigquit) {
            pthread_mutex_lock(&totalCLientsMTX);
            totalClients--;
            pthread_mutex_unlock(&totalCLientsMTX);
            return NULL;
        }
        
        // salvo su j l'indice della cassa scelta (nella struttura cashiers)
        j = clients[i].cashiersIndex;
        
        
        // se il cliente non è ancora primo in coda aspetto che lo diventi
        pthread_mutex_lock(&enterServiceMTX[j]);
        if(clients[i].posInQueue > 1) {
            pthread_cond_wait(&firstPosClientCOND[i], &enterServiceMTX[j]);
        }
        
        // il cliente paga
        clients[i].hasPaid = true;
        
        // mando il segnale che avvisa il cassiere che è presente un cliente in coda
        // (se non aspetto sulla wait significa che sta aspettando il cassiere, ovvero nessuno è
        // in coda)
        pthread_cond_signal(&cashiersServiceCOND[j]);
        
        // se il cliente non è stato ancora autorizzato ad uscire, aspetto
                
        if(clients[i].isOut != true) {
            pthread_cond_wait(&clientIsOutCOND[i], &enterServiceMTX[j]);
        }
        
        // mando il segnale di avvenuto pagamento
        pthread_cond_signal(&clientHasPaidCOND[j]);
        pthread_mutex_unlock(&enterServiceMTX[j]);
    
        // se il cliente deve cambiare cassa perchè essa è stata chiusa resetto le seguenti
        // variabili che permettono al thread di ciclare nuovamente sul while e al cliente di
        // scegliere un'altra cassa
        if(clients[i].isChanging == true) {
            clients[i].isOut = false;
            clients[i].isChanging = false;
            clients[i].hasPaid = false;
            clients[i].posInQueue = config.C + 1;
        }
        // se è stato segnalato SIGQUIT o se un nuovo cliente è entrato durante la chiusura del SM
        // decremento il numero dei clienti presenti all'interno del SM ed esco
        double serviceTime = (clients[i].enterService.tv_sec);
        serviceTime += (clients[i].enterService.tv_nsec) / 1000000000.0;
        
        if(sigquit && (serviceTime == 0.0 || i == -1)) {
            pthread_mutex_lock(&totalCLientsMTX);
            totalClients--;
            pthread_mutex_unlock(&totalCLientsMTX);
            return NULL;
        }
        
        
    }
    

    // prendo il tempo di uscita del cliente dal SM
    clock_gettime(CLOCK_MONOTONIC, &clients[i].exitSM);

    // calcolo i tempi come double
    double totTime = (clients[i].exitSM.tv_sec - clients[i].enterSM.tv_sec);
    totTime += (clients[i].exitSM.tv_nsec - clients[i].enterSM.tv_nsec) / 1000000000.0;
    double serviceTime = (clients[i].enterService.tv_sec);
    serviceTime += (clients[i].enterService.tv_nsec) / 1000000000.0;
    double queueTime = totTime - serviceTime - buyingTime / 1000.0;
   
    
    // stampo sul file di log dei clienti
    fprintf(statsClients, "Sono il cliente %ld, ho comprato %d oggetti e ho impiegato %.3f s, di cui %.3f s per comprare i prodotti, %.3f s in coda e %.3f s tempo di servizio. Cassa assegnata numero: %d, code cambiate: %d\n",
        (long) clients[i].index,
        clients[i].nProducts,
        totTime,
        buyingTime / 1000.0,
        queueTime,
        serviceTime,
        clients[i].cashiersIndex,
            clients[i].nQueueChanges);
    
    // scrivo -1 sull'index del cliente in uscita per discriminarlo da coloro ancora presenti all'interno del SM
    clients[i].index = -1;
    
    // incremento il numero dei clienti usciti
    exitedClients++;
    // se i clienti usciti sono E mando il segnale per farni entrare altri E
    pthread_mutex_lock(&newClientsMTX);
    if(exitedClients == config.E) {
        pthread_cond_signal(&newClientsCOND);
    }
    pthread_mutex_unlock(&newClientsMTX);
    
    // decremento il numero dei clienti presenti all'interno del SM
    pthread_mutex_lock(&totalCLientsMTX);
    totalClients--;
    pthread_mutex_unlock(&totalCLientsMTX);

    
    // esco
    return NULL;
}
