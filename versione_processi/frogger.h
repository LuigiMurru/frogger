#ifndef FROGGER_H
#define FROGGER_H


// Librerie
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>


// Costanti di gioco
#define NUM_CORSIE_FIUME 8
#define NUM_COCCODRILLI (NUM_CORSIE_FIUME * 2)
#define NUM_TANE 5
#define DELAY 100000
#define LARGHEZZA_RANA 2
#define ALTEZZA_RANA 2
#define LARGHEZZA_COCCODRILLO 6
#define ALTEZZA_COCCODRILLO 2


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
- K è la kill, mandata da granate e proiettili quando collidono
- Z rimuove ogni singola granata e proiettile
- U aggiorna la lista proiettili e granate da processi.c a gioco.c
*/


// Strutture dati
typedef struct {
    char tipo;
    long id;
    int x, y, dx;
} Messaggio;

typedef struct {
    char tipo;
    int x, y, dx;
} Comando;

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

typedef struct NodoIdIgnorati {
    long id;
    int tempo_vita;
    struct NodoIdIgnorati *next;
} NodoIdIgnorati;


// Pipe da produttori a consumatore
extern int pipe_produttori_a_consumatore[2]; 

// Pipe da consumatore a produttori
extern int pipe_a_cocco[2];
extern int pipe_a_rana[2];
extern int pipe_a_gestore_proiettili[2];
extern int pipe_a_gestore_granate[2];


// Variabili globali (esterne)
extern int max_x, max_y, rana_x, rana_y;
extern int cocco_x[NUM_COCCODRILLI], vecchio_cocco_x[NUM_COCCODRILLI], cocco_y[NUM_COCCODRILLI], cocco_sparo_timer[NUM_COCCODRILLI];
extern int vite, timer, partita_finita, punteggio;
extern int id_cocco_cavalcato, offset_rana;
extern int tane_x[NUM_TANE], tane_occupate[NUM_TANE], tane_raggiunte;
extern int animazione_acqua_timer;
extern char buffer_acqua[64][256];
extern NodoProiettile *lista_proiettili;
extern NodoIdIgnorati *lista_id_ignorati;
extern ImpostazioniDifficolta impostazioni_correnti;
extern time_t ultimo_lancio_granata_t;

// collisioni.c
bool collisione_rettangoli(int r1x, int r1y, int r1w, int r1h, int r2x, int r2y, int r2w, int r2h);
void aggiungi_a_ignorati(long id_da_ignorare);
void gestione_collisioni();

// gioco.c
void start_rana();
void reset_rana(int pipe_a_cocco, int pipe_produttori_a_consumatore, int pipe_a_gestore_proiettili, int pipe_a_gestore_granate);
void gioca();

// interfaccia.c
void disegna_interfaccia();
void difficolta();
void istruzioni();
void crediti();
void pausa();
int leggi_record();
int scrivi_record(int nuovo_record);
void fine_partita();

// main.c
bool avvia_sessione();
int mostra_menu();

// processi.c
void processo_coccodrillo(int id, int start_x, int y, int max_x_val, float moltiplicatore_velocita, int pipe_out, int pipe_in);
void processo_rana(int pipe_stato_in, int pipe_eventi_out);
void processo_gestore_proiettili(int pipe_in, int pipe_out, int max_x_val);
void processo_gestore_granate(int pipe_in, int pipe_out, int max_x_val);

// proiettili.c
void aggiorna_proiettile(Messaggio msg);
void rimuovi_p_g();

#endif