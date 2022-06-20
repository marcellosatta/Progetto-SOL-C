#!/bin/bash

# SCRIPT DI ANALISI DEI FILE DI LOG: sono stati creati due file, il primo (in
# ordine di parsing) riguarda i dati dei clienti, il secondo i dati dei cassieri.

# Il formato è il seguente, per i clienti:

    # ID cliente: XXX | n. prodotti: XXX | tot. tempo: XXX s | tempo acquisti: XXX s | tempo in coda: XXX s | tempo di servizio: XXX s | n. cassa: XXX | n. code cambiate: XXX
    
# per i cassieri:

    # ID cassiere: XXX | n. prodotti elaborati: XXX | n. clienti serviti: XXX | tot. tempo: XXX s | tempo di servizio medio: XXX s | n. chiusure cassa: XXX


# controllo se il file dove sono immagazzinati i dati dei clienti è presente
if [ -f "./statsClients.txt" ]; then
    # leggo riga per riga il file
    while read line;
        # sostituisco alla verbosità delle stringhe stampate su file una formattazione
        # più intuitiva
        do
        line=${line/"Sono il cliente "/"ID cliente: "};
        line=${line/", ho comprato "/" | n. prodotti: "};
        line=${line/" oggetti e ho impiegato "/" | tot. tempo: "};
        line=${line/", di cui "/" | tempo acquisti: "};
        line=${line/" per comprare i prodotti, "/" | tempo in coda: "};
        line=${line/" in coda e "/" | tempo di servizio: "};
        line=${line/" tempo di servizio. Cassa assegnata numero: "/" | n. cassa: "};
        line=${line/", code cambiate: "/" | n. code cambiate: "};
        # stampo su terminale
        echo $line;
        # sleep di 0.1s (permette di aspettare che tutte le stringhe vengano
        # stampate su file, ovvero che tutti i clienti siano usciti dal supermercato)
        sleep 0.1s;
        done < statsClients.txt
else
    echo "Errore di lettura del file statsClients.txt!"
fi

# controllo se il file dove sono immagazzinati i dati dei cassieri è presente
if [ -f "./statsCashiers.txt" ]; then
    # leggo riga per riga il file
    while read line;
        # sostituisco alla verbosità delle stringhe stampate su file una formattazione
        # più intuitiva
        do
        line=${line/"Sono il cassiere "/"ID cassiere: "};
        line=${line/", ho elaborato "/" | n. prodotti elaborati: "};
        line=${line/" prodotti acquistati da "/" | n. clienti serviti: "};
        line=${line/" clienti. Ho lavorato per "/" | tot. tempo: "};
        line=${line/" con un tempo di servizio medio di "/" | tempo di servizio medio: "};
        line=${line/". Ho chiuso la cassa "/" | n. chiusure cassa: "};
        line=${line/" volte"/""};
        # stampo su terminale
        echo $line;
        # sleep di 0.1s (permette di aspettare che tutte le stringhe vengano
        # stampate su file, ovvero che tutti i cassieri abbiano terminato il proprio lavoro)
        sleep 0.1s;
        done < statsCashiers.txt
else
    echo "Errore di lettura del file statsCashiers.txt!"
fi
