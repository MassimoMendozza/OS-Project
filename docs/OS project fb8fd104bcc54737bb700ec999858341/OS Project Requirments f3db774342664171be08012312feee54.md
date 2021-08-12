# OS Project Requirments

## Requirements

      Parameters

- Grid SO_WIDTH x SO_HEIGHT
- SO_HOLES celle inaccessibili circondate da celle accessibili
- Tempo di attraversamento random fra SO_MIN_NANOSEC e SO_MAX_NANOSEC
- Ogni cella ha capacità di taxi estratta randomicamente fra SO_CAP_MIN e SO_CAP_MAX
- Caratteristiche random delle celle generate al lancio master e mai più cambiate

       Richieste di servizio

- Richieste taxi generate da SO_SOURCES processi lanciati da master, non sovrapposte, messe in coda di messaggi o pipe (avere singola coda/pipe per processo oppure una per tutti a piacimento)
- Richiesta taxi: cella di partenza della richiesta, cella di destinazione casuale
- Richieste create secondo pattern configurabile a mia scelta, tipo con intervallo variabile o alarm periodico
- In più ogni processo generatore deve generare una richiesta ogni volta che riceve un segnale (ad esempio da terminale)
- Taxi preleva richiesta quando è su stessa cella

      Movimento Taxi

- Taxi posizionati in maniera casuale
- Abilitati alla ricerca dopo che tutti gli altri processi taxi sono stati creati ed inizializzati
- Se un taxi non si muove entro SO_TIMEOUT secondi termina rilasciando eventuali risorse. La richiesta viene marcata come ABORTED, terminata ed un nuovo processo taxi viene creato.

       Inizio/Terminazione simulazione

- Inizia una volta che tutti i processi taxi e sorgente sono inizializzati
- Simulazione ha durata SO_DURATION e sarà poi un alarm a master a determinare la fine
- Ogni secondo master stampa stato occupazione celle
- A fine simulazione si stampano:
- Numeri viaggi con successo, inevasi e abortiti
- Mappa con evidenziate le SO_SOURCES sorgenti e le SO_TOP_CELLS celle più attraversate 
- Processo taxi che:
1) Ha fatto più strada
2) Ha fatto il viaggio più lungo temporalmente nel servire una richiesta
3) Ha raccolto più richieste e/o clienti

       Valori di esempio

![OS%20Project%20Requirments%20f3db774342664171be08012312feee54/Untitled.png](OS%20Project%20Requirments%20f3db774342664171be08012312feee54/Untitled.png)

       Requisiti implementativi

- Divisione del codice in moduli
- Compilato con make
- Massimizzare concorrenza tra processi
- Deallocare risorse IPC al termine del gioco
- Compilati con

```bash
gcc -std=c89 -pedantic
```

- Eseguirlo correttamente su macchina virtuale o fisica che presenta parallelismo