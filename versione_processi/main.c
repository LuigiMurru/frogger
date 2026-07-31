#include "frogger.h"

// Variabili globali
int max_x, max_y, rana_x, rana_y;
int cocco_x[NUM_COCCODRILLI], vecchio_cocco_x[NUM_COCCODRILLI], cocco_y[NUM_COCCODRILLI];
int cocco_sparo_timer[NUM_COCCODRILLI] = {0};
int vite, timer, partita_finita = 0, punteggio = 0;
int id_cocco_cavalcato = -1, offset_rana = 0;
int tane_x[NUM_TANE], tane_occupate[NUM_TANE] = {0}, tane_raggiunte = 0;
NodoProiettile *lista_proiettili = NULL;
NodoIdIgnorati *lista_id_ignorati = NULL;
ImpostazioniDifficolta impostazioni_correnti;
time_t ultimo_lancio_granata_t = 0;

// Pipe
int pipe_produttori_a_consumatore[2];
int pipe_a_rana[2];
int pipe_a_gestore_proiettili[2];
int pipe_a_gestore_granate[2];
int pipe_a_cocco[2];

// Sprite globali
const char* SPRITE_RANA[ALTEZZA_RANA] = {"\\/", "/\\"};
const char* SPRITE_COCCO_DESTRA[ALTEZZA_COCCODRILLO] = {"~^~^~", "-----="};
const char* SPRITE_COCCO_SINISTRA[ALTEZZA_COCCODRILLO] = {"~^~^~", "=-----"};
const char* SPRITE_COCCO_DESTRA_SPARO[ALTEZZA_COCCODRILLO] = {"^^^^^", "-----<"};
const char* SPRITE_COCCO_SINISTRA_SPARO[ALTEZZA_COCCODRILLO] = {"^^^^^", ">-----"};
const char* NOME_FILE_RECORD = "record.txt";

// Variabili animazioni dell'acqua
char buffer_acqua[64][256];
int animazione_acqua_timer = 0;


// Main
int main() {
    srand(time(NULL));
    while (avvia_sessione());
    return 0;
}


bool avvia_sessione() {
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(COPPIA_COLORE_RANA, COLOR_GREEN, COLOR_BLACK);
    init_pair(COPPIA_COLORE_ACQUA, COLOR_CYAN, COLOR_BLUE);
    init_pair(COPPIA_COLORE_MARCIAPIEDE, COLOR_BLACK, COLOR_GREEN);
    init_pair(COPPIA_COLORE_TANA, COLOR_BLACK, COLOR_YELLOW);
    init_pair(COPPIA_COLORE_COCCODRILLO, COLOR_WHITE, COLOR_GREEN);
    init_pair(COPPIA_COLORE_PROIETTILE, COLOR_RED, COLOR_BLACK);
    init_pair(COPPIA_COLORE_GRANATA, COLOR_YELLOW, COLOR_BLACK);
    
    int scelta = mostra_menu();
    switch (scelta) {
        case '1':
            difficolta();
            break;
        case '2':
            istruzioni();
            break;
        case '3':
            crediti();
            break;
        case '4':
            endwin();
            return false;
    }
    return true;
}


int mostra_menu() {
    getmaxyx(stdscr, max_y, max_x);
    clear();
    mvprintw(max_y/2 - 4, (max_x - 21)/2, "Frogger: Resurrection");
    mvprintw(max_y/2, (max_x - 19)/2, "1 -> Inizia Partita");
    mvprintw(max_y/2 + 1, (max_x - 19)/2, "2 -> Istruzioni");
    mvprintw(max_y/2 + 2, (max_x - 19)/2, "3 -> Crediti");
    mvprintw(max_y/2 + 3, (max_x - 19)/2, "4 -> Esci");
    refresh();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    return getch();
}