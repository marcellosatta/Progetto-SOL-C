#!/bin/sh

# TEST2: il seguente test legge gli imput dal file config2.txt

# 6 K // numero casse
# 50 C // numero clienti che entrano in un primo momento
# 3 E // numero di clienti che entrano all'uscita di altrettanti
# 200 T // tempo massimo speso per acquistare i prodotti da parte dei clienti
# 10 L // tempo di servizio di un cassiere per singolo prodotto
# 100 P // massimo numero di prodotti acquistabili
# 20 S // intervallo di tempo in cui un cliente controlla la coda di una nuova cassa
# 3 S1 // soglia di chiusura cassa da parte del direttore
# 12 S2 // soglia di apertura nuova cassa
# 3 openCD // n casse aperte inizialmente
# 300 refreshD // intervallo di tempo in cui un cassiere aggiorna il direttore sul numero dei clienti in coda
# statsClients.txt logname1 // nome file di log per i clienti
# statsCashiers.txt logname2 // nome file di log per i cassieri

echo
echo "                                                     - SUPERMERCATO SOL - MARCELLO SATTA - 580495 - CORSO B -"
echo
echo "                                                                            - TEST 2: "
echo
echo "                                                                          - N° casse: 6 -"
echo "                                                           - N° clienti all'ingresso: 50 -"
echo "                                  - N° clienti che entrano all'uscita di altrettanti: 3 -"
echo "                                                                - T. max di acquisto: 0.2 s -"
echo "                                               - T. di servizio per singolo prodotto: 0.01 s -"
echo "                                                      - N° max prodotti acquistabili: 100 -"
echo "                         - T. in cui un cliente controlla la coda di una nuova cassa: 0.02 s -"
echo "                                                             - Soglia chiusura cassa: 3 -"
echo "                                                             - Soglia apertura cassa: 12 -"
echo "                                                      - N° casse aperte inizialmente: 3 -"
echo "        - T. in cui un cassiere aggiorna il direttore sul numero dei clienti in coda: 0.3 s -"
echo "                                                               - File di log clienti: statsClients.txt -"
echo "                                                              - File di log cassieri: statsCashiers.txt -"
echo
echo

# viene lanciato l'eseguibile e il PID del processo director viene salvato in director.PID
((./director ./config2.txt) & echo "$!" > director.PID) &
echo "                                        - Il programma è stato lanciato: i risultati saranno disponibili tra 25 secondi. -"
echo &
# contemporaneamente viene lanciata la sleep di 25 s
sleep 25s
# viene lanciato il segnale di SIGHUP al processo director
kill -1 $( cat director.PID )
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


