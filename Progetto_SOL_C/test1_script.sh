#!/bin/sh

# TEST1: il seguente test legge gli imput dal file config1.txt

# 2 K // numero casse
# 20 C // numero clienti che entrano in un primo momento
# 5 E // numero di clienti che entrano all'uscita di altrettanti
# 500 T // tempo massimo speso per acquistare i prodotti da parte dei clienti
# 10 L // tempo di servizio di un cassiere per singolo prodotto
# 80 P // massimo numero di prodotti acquistabili
# 30 S // intervallo di tempo in cui un cliente controlla la coda di una nuova cassa
# 2 S1 // soglia di chiusura cassa da parte del direttore
# 8 S2 // soglia di apertura nuova cassa
# 1 openCD // n casse aperte inizialmente
# 700 refreshD // intervallo di tempo in cui un cassiere aggiorna il direttore sul numero dei clienti in coda
# statsClients.txt logname1 // nome file di log per i clienti
# statsCashiers.txt logname2 // nome file di log per i cassieri

echo
echo "                                                     - SUPERMERCATO SOL - MARCELLO SATTA - 580495 - CORSO B -"
echo
echo "                                                                            - TEST 1: "
echo
echo "                                                                          - N° casse: 2 -"
echo "                                                           - N° clienti all'ingresso: 20 -"
echo "                                  - N° clienti che entrano all'uscita di altrettanti: 5 -"
echo "                                                                - T. max di acquisto: 0.5 s -"
echo "                                               - T. di servizio per singolo prodotto: 0.01 s -"
echo "                                                      - N° max prodotti acquistabili: 80 -"
echo "                         - T. in cui un cliente controlla la coda di una nuova cassa: 0.03 s -"
echo "                                                             - Soglia chiusura cassa: 2 -"
echo "                                                             - Soglia apertura cassa: 8 -"
echo "                                                      - N° casse aperte inizialmente: 1 -"
echo "        - T. in cui un cassiere aggiorna il direttore sul numero dei clienti in coda: 0.7 s -"
echo "                                                               - File di log clienti: statsClients.txt -"
echo "                                                              - File di log cassieri: statsCashiers.txt -"
echo
echo

# viene lanciato l'eseguibile tramite valgrind e il PID del processo director viene salvato in director.PID
((valgrind --leak-check=full ./director ./config1.txt) & echo "$!" > director.PID) &
echo "                                        - Il programma è stato lanciato: i risultati saranno disponibili tra 15 secondi. -"
echo &
# contemporaneamente viene lanciata la sleep di 15 s
sleep 15s
# viene lanciato il segnale di SIGQUIT al processo director
kill -3 $( cat director.PID )
# attendo che il processo termini
while [ -e /proc/director.PID ]
do
    sleep 1s
done
# lancio lo script di analisi (./analysis.sh)
chmod +x ./analysis.sh
./analysis.sh
# clean-up dei file .PID, file socket e .dSYM
rm -rf director.PID socketSM_D director.dSYM
echo
echo "                                                           - Il programma è terminato correttamente! -"
echo
