#ifndef FROGGER_H
#define FROGGER_H


// Librerie
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>


// Costanti di gioco
#define NUM_CORSIE_FIUME 8
#define NUM_COCCODRILLI (NUM_CORSIE_FIUME * 2)
#define NUM_TANE 5
#define DELAY 100000
#define LARGHEZZA_RANA 2
#define ALTEZZA_RANA 2
#define LARGHEZZA_COCCODRILLO 6
#define ALTEZZA_COCCODRILLO 2
#define BUFFER_SIZE 256


// Sprite
extern const char* SPRITE_RANA[ALTEZZA_RANA];
extern const char* SPRITE_COCCO_DESTRA[ALTEZZA_COCCODRILLO];
extern const char* SPRITE_COCCO_SINISTRA[ALTEZZA_COCCODRILLO];
extern const char* SPRITE_COCCO_DESTRA_SPARO[ALTEZZA_COCCODRILLO];
extern const char* SPRITE_COCCO_SINISTRA_SPARO[ALTEZZA_COCCODRILLO];
extern const char* NOME_FILE_RECORD;


// Coppie cromatiche
#define COPPIA_COLORE_RANA 1
#define COPPIA_COLORE_ACQUA 2
#define COPPIA_COLORE_MARCIAPIEDE 3
#define COPPIA_COLORE_TANA 4
#define COPPIA_COLORE_COCCODRILLO 5
#define COPPIA_COLORE_PROIETTILE 6
#define COPPIA_COLORE_GRANATA 7



//Lettere dei tipi:
/*
- C coccodrillo
- S richiesta di sparo, che manda una P in gestore_proiettili
- M movimento rana
- L richiesta di lancio, che manda una G in gestore_granate
- K è la kill di una granata o proiettile con ID specifico
- D è la distruzione di un proiettile o granata quando esce dallo schermo
- Z rimuove ogni singola granata e proiettile
- U aggiorna la lista proiettili e granate da processi.c a gioco.c
*/


// Strutture dati
typedef struct { char tipo; long id; int x, y, dx; } Messaggio;

typedef struct NodoProiettile {
    long id;
    char tipo;
    int x, y, dx;
    bool da_rimuovere;
    struct NodoProiettile *next;
} NodoProiettile;

typedef struct { 
    int vite; 
    int tempo_gioco; 
    float moltiplicatore_velocita; 
} ImpostazioniDifficolta;

typedef struct { 
    Messaggio buffer[BUFFER_SIZE]; 
    int head, tail; 
    sem_t full, empty; 
    pthread_mutex_t mutex; 
} BufferCondiviso;

typedef struct ComandoNodo { 
    Messaggio comando; 
    struct ComandoNodo *next; 
} ComandoNodo;

typedef struct { 
    ComandoNodo *head, *tail; 
    pthread_mutex_t mutex; 
} ComandoCoda;

typedef struct { 
    int id; 
    int start_x; 
    int y; 
    float moltiplicatore_velocita; 
} Coccodrillo;

typedef struct { 
    int max_x_val; 
} GranateProiettili;


// Variabili globali
extern int max_x, max_y, rana_x, rana_y;
extern int cocco_x[NUM_COCCODRILLI], cocco_y[NUM_COCCODRILLI], cocco_sparo_timer[NUM_COCCODRILLI];
extern int vite, timer, punteggio;
extern bool partita_finita;
extern int id_cocco_cavalcato, offset_rana;
extern int tane_x[NUM_TANE], tane_occupate[NUM_TANE], tane_raggiunte;
extern NodoProiettile *lista_proiettili;
extern ImpostazioniDifficolta impostazioni_correnti;
extern time_t ultimo_lancio_granata_t;
extern char buffer_acqua[64][256];
extern int animazione_acqua_timer;


// Risorse concorrenza
extern BufferCondiviso buffer_comunicazione;
extern ComandoCoda coda_proiettili;
extern ComandoCoda coda_granate;
extern pthread_mutex_t ncurses_mutex;
extern volatile bool partita_in_corso;


// collisioni.c
bool collisione_rettangoli(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
void gestione_collisioni();

// gioco.c
void start_rana();
void reset_rana();
void gioca();

void produci(Messaggio msg);
void accoda_comando(ComandoCoda *coda, Messaggio msg);
bool preleva_comando(ComandoCoda *coda, Messaggio *msg);

// interfaccia.c
void disegna_interfaccia();
void difficolta();
void istruzioni();
void crediti();
void pausa();
int leggi_record();
int scrivi_record(int nuovo_record);
void fine_partita();

//main.c
void inizializza_buffer();
void distruggi_buffer();
void produci(Messaggio msg);
void inizializza_coda(ComandoCoda *coda);
void distruggi_coda(ComandoCoda *coda);
void accoda_comando(ComandoCoda *coda, Messaggio msg);
bool preleva_comando(ComandoCoda *coda, Messaggio *msg);
bool avvia_sessione();
int mostra_menu();

// proiettili.c
void aggiorna_proiettile(Messaggio msg);
void rimuovi_p_g();

// threads.c
void* thread_coccodrillo(void *args);
void* thread_rana(void *args);
void* thread_gestore_proiettili(void *args);
void* thread_gestore_granate(void *args);





#endif