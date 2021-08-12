# OS Project Subdivision

master.c

- Supervisione per gestione statistiche
- Inizializzazione risorse IPC
    - Inizializzazione mappa
    - Inizializzazione code di messaggi
    - Inizializzazione semafori
    - Inizializzazione processi
- Gestione alarm
- Controllo temporizzazione taxi
- Raccolta statistiche
- Gestione segnale per richiesta
- Stampa della mappa

 meccanismi di comunicazione

- Nella realtà i taxi comunicano la corsa presa secondo un loro canale, protocollo di pacchetti?
- Memmap assolutamente per le celle(?)
- Start and stop dei processi

taxi.c

- Movimento in cella
- Se è idle è in ascolto per le richieste
- Controlla lui se può andare oppure chiede a processo master?

source.c

- Immissione delle richieste nella coda
- comunicazione della propria posizione