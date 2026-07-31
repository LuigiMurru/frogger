# Frogger: Resurrection

Clone testuale di Frogger scritto in C con `ncurses`, realizzato in due versioni che implementano la stessa logica di gioco con due modelli di concorrenza diversi:

- **`versione_processi/`** — architettura multi-processo: coccodrilli, rana, gestore proiettili e gestore granate sono processi separati (`fork`) che comunicano tramite **pipe**.
- **`versione_thread/`** — architettura multi-thread: gli stessi componenti sono **thread POSIX** che comunicano tramite un buffer condiviso produttore/consumatore, mutex e semafori.

## Come si gioca

- Frecce direzionali per muovere la rana, obiettivo: raggiungere le 5 tane in alto.
- Barra spaziatrice per lanciare granate (max. 1 al secondo).
- `Q` per il menu di pausa.
- Punteggio: +50 per tana raggiunta, -10 per ogni morte, bonus per il tempo rimanente.

## Compilazione ed esecuzione

Richiede `gcc` e la libreria `ncurses` (`libncurses-dev` su Debian/Ubuntu).

```bash
cd versione_processi   # oppure versione_thread
make
./frogger
```

## Crediti

Progetto a cura di **Daniele Dessì** e **Luigi Murru**, per il corso di *Sistemi Operativi e Programmazione di Rete*.
