#ifndef supermarket_h
#define supermarket_h

#include <stdio.h>

// handler SIGHUP
static void SIGHUPhandler(int);
// handler SIGQUIT
static void SIGQUIThandler(int);

// funzione di controllo thread cassiere
void* openCashiersDesk(long);
// funzione di controllo thread cliente
void* clientEnterSM(long);

#endif /* supermarket_h */
