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

#include "cnfg.h"
#include "director.h"

#define N 256
#define SOCKNAME "socketSM_D"
#define UNIX_PATH_MAX 100

// variabile segnali SIGQUIT e SUGHUP
static volatile sig_atomic_t sigquit=0;
static volatile sig_atomic_t sighup=0;

// PID processo supermercato
static int pidSM;

// handler SIGQUIT
static void SIGQUIThandler(int x) {
    sigquit=1;
    kill(pidSM, SIGQUIT);

}

//handler SIGHUP
static void SIGHUPhandler(int x) {
    sighup=1;
    kill(pidSM, SIGHUP);
}



// funzione che inizializza la strutturas di controllo del supermercato da parte del direttore
supermarket* initializeSM(int k, int openCD) {
    supermarket* sm = malloc(sizeof(supermarket));
    sm->cashDesks = malloc(sizeof(cashDesk)* k);
    for(int i = 0; i < k; i++) {
        if(i < openCD)
            sm->cashDesks[i].open = true;
        else
            sm->cashDesks[i].open = false;
        sm->cashDesks[i].nClients = -1;
        }
    sm->nOpenCashDesk = openCD;
    return sm;
}


// struttura dati di controllo del supermercato da parte del direttore
static supermarket* sm;
static cnfg config;

int main(int argc, char* argv[]) {
    
    // controllo parametri
    if(argc != 2) {
        perror("Numero di argomenti errato!\n");
        fflush(stderr);
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
    
    signal(SIGPIPE, SIG_IGN);
    
    // file descriptor per il socket
    int directorFD;
    
    // creazione SOCKET di comunicazione tra processo Direttore e processo Supermercato
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    strncpy(address.sun_path, SOCKNAME, strlen(SOCKNAME));
    address.sun_family=AF_UNIX;
   
    if((directorFD=socket(AF_UNIX,SOCK_STREAM,0)) == -1) {
        perror("Errore durante la socket()!\n");
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(directorFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
    
    if(bind(directorFD,(struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Errore durante la bind()!");
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
   
    if(listen(directorFD, 32) == -1) {
        perror("Errore durante la listen()!\n");
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
    
    // creazione set per funzione select()
    fd_set set, tmpset;
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(directorFD, &set);
    
    long fdmax = directorFD;

    
    // fork e execl processo supermercato
    
    pidSM = fork();
    
    if (pidSM == 0) {
        execl("./supermarket", "supermarket", argv[1], NULL);
        perror("Errore execl()!\n");
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
    else if (pidSM < 0){
        // se la fork() restituisce un valore negativo esco, la fork è fallita
        fprintf(stderr, "ERRORE: fork() fallita!\n");
        exit(EXIT_FAILURE);
    }
    else {
        //lettura parametri di configurazione
        if(readConfig(&(config), argv[1]) != 0) {
            perror("Errore durante la lettura del file di configurazione.\n");
            fflush(stderr);

            exit(EXIT_FAILURE);
        }
        
        // inizializzazione struttura di controllo del supermercato da parte del direttore
        sm = initializeSM(config.K, config.openCD);
        
        // maschera dei segnali per la select()
        sigset_t mask;
        sigset_t orig_mask;
        sigemptyset (&mask);
        sigemptyset (&orig_mask);
        sigaddset (&mask, SIGQUIT);
        sigaddset (&mask, SIGHUP);
        
        while(1) {
            
            // durante la select() maschero i segnali, altrimenti potrebbe venire interrotta
            if (pthread_sigmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
                perror ("pthread_sigmask error\n");
                return 1;
            }
            
            // select()
            tmpset = set;
            if(select((int)fdmax+1, &tmpset, NULL, NULL, NULL) == -1) {
                perror("select");
                fflush(stderr);

                exit(EXIT_FAILURE);
            }
            // setto la maschera dei segnali originale
            if (pthread_sigmask(SIG_SETMASK, &orig_mask, NULL) < 0) {
                perror ("pthread_sigmask error\n");
                return 1;
            }
            
            // cerco di capire chi ha fatto richiesta di scrittura sul socket
            for(int i=0; i <= fdmax; i++) {
                
                if (FD_ISSET(i, &tmpset)) {
                    long connfd;
                    if (i == directorFD) {
                        connfd = accept(directorFD, (struct sockaddr*)NULL ,NULL);
                        FD_SET(connfd, &set);
                        if(connfd > fdmax) fdmax = connfd;
                            continue;
                    }
                    
                    connfd = i;
                    // apertura o chiusura cassa
                    int c = directorsControl(connfd);
                    if(c < 0) {
                        close((int) connfd);
                        FD_CLR(connfd, &set);
                        if (connfd == fdmax)
                            fdmax = updatemax(set, (int)fdmax);
                    }
                    // terminazione processo direttore
                    else if (c == 3) {
                        free(sm->cashDesks);
                        free(sm);
                        unlink(SOCKNAME);
                        return 0;
                    }
                }
            }
        }
    }
}

// apertura o chiusura cassa
int directorsControl(long connfd) {
    // leggo il numero di clienti in cassa
    char buf[N];
    memset(&buf, 0, sizeof(char) * N);
    long n = read((int)connfd,buf,N);

    if (n < 0)
        return -1;

    int k = 0;
    char* token;
    char* rest = buf;
    bool isCashier = 0;
    int cashierIndex = -1;
    int nClientsInQueue = -1;
    
    // se un cliente vuole uscire scrive 0 e il direttore risponde 1
    if(strncmp("0", buf, strlen(buf)) == 0) {
        memset(&buf, 0, sizeof(char) * N);
        sprintf(buf, "1");
        n = write((int)connfd, buf, sizeof(char) * strlen(buf));
        if (n < 0)
            return -1;
        return 0;
    }
    // se il supermercato sta chiudendo scrive close e il direttore risponde ok
    else if(strncmp("close", buf, strlen(buf)) == 0) {
        memset(&buf, 0, sizeof(char) * N);
        sprintf(buf, "ok");
        n = write((int)connfd, buf, sizeof(char) * strlen(buf));
        if (n < 0)
            return -1;
        memset(&buf, 0, sizeof(char) * N);
        return 3;
    }
    
    // apertura o chiusura cassa
    // leggo e riconosco il cassiere e il numero dei clienti in cassa
    while ((token = strtok_r(rest, "_", &rest)) && k < 3) {
        if(k == 0 && strncmp("cashier", token, strlen(token)) == 0)
            isCashier = 1;
        else if(k == 1 && isCashier == 1) {
            if(cashierIndex == -1)
                cashierIndex = atoi(token);
        }
        else if(k == 2 && cashierIndex > -1)
                nClientsInQueue = atoi(token);
        k++;
    }
    
    // chiudo una cassa se ci sono almeno S1 casse con al più un cliente
    if(isCashier == true && nClientsInQueue <= 1) {
        int counter = 1;
        for(int i = 0; i < config.K; i++) {
            if(sm->cashDesks[i].open == true && sm->cashDesks[i].nClients == 1 && i != cashierIndex)
                counter++;
        }
        if(counter >= config.S1 && sm->nOpenCashDesk > 1) {
            sm->cashDesks[cashierIndex].open = false;
            sm->cashDesks[cashierIndex].nClients = -1;
            sm->nOpenCashDesk--;
            memset(&buf, 0, sizeof(char) * N);
            sprintf(buf, "close_%d_", cashierIndex);
            
            n = write((int) connfd, buf, sizeof(char) * strlen(buf));
            if (n < 0)
                return -1;
            return 0;
        }
    }
    // apro una nuova cassa se la cassa analizzata possiede più di S2 clienti in coda
    else if(isCashier == true && nClientsInQueue >= config.S2) {
        for(int i = 0; i < config.K; i++) {
            if(sm->cashDesks[i].open == false) {
                sm->cashDesks[cashierIndex].open = true;
                sm->nOpenCashDesk++;
                memset(&buf, 0, sizeof(char) * N);
                sprintf(buf, "open_%d_", i);
                    
                n = write((int)connfd, buf, sizeof(char) * strlen(buf));
                if (n < 0)
                    return -1;
                return 0;
            }
        }
    }
    
    // scrivo null se il direttore non decide ne' di aprire ne' di chiudere
    memset(&buf, 0, sizeof(char) * N);
    sprintf(buf, "null");
    n = write((int)connfd, buf, sizeof(char) * strlen(buf));
    if (n < 0)
        return -1;
    return 0;
}

int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
        if (FD_ISSET(i, &set))
            return i;
        assert(1==0);
        return -1;
}

