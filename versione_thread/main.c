#include "frogger.h"

// Variabili globali
int max_x, max_y, rana_x, rana_y;
int cocco_x[NUM_COCCODRILLI], cocco_y[NUM_COCCODRILLI], cocco_sparo_timer[NUM_COCCODRILLI] = {0};
int vite, timer, punteggio = 0;
bool partita_finita = false;
int id_cocco_cavalcato = -1, offset_rana = 0;
int tane_x[NUM_TANE], tane_occupate[NUM_TANE] = {0}, tane_raggiunte = 0;
NodoProiettile *lista_proiettili = NULL;
ImpostazioniDifficolta impostazioni_correnti;
time_t ultimo_lancio_granata_t = 0;
BufferCondiviso buffer_comunicazione;
ComandoCoda coda_proiettili;
ComandoCoda coda_granate;
pthread_mutex_t ncurses_mutex;
volatile bool partita_in_corso = false;

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


// Buffer
void inizializza_buffer() {
    buffer_comunicazione.head = 0; buffer_comunicazione.tail = 0;
    pthread_mutex_init(&buffer_comunicazione.mutex, NULL);
    sem_init(&buffer_comunicazione.empty, 0, BUFFER_SIZE);
    sem_init(&buffer_comunicazione.full, 0, 0);
}


// Distrugge risorse buffer quando non servono più
void distruggi_buffer() {
    pthread_mutex_destroy(&buffer_comunicazione.mutex);
    sem_destroy(&buffer_comunicazione.empty);
    sem_destroy(&buffer_comunicazione.full);
}


// Acquisisce il mutex per scrivere nel buffer
void produci(Messaggio msg) {
    sem_wait(&buffer_comunicazione.empty);
    pthread_mutex_lock(&buffer_comunicazione.mutex);
    buffer_comunicazione.buffer[buffer_comunicazione.head] = msg;
    buffer_comunicazione.head = (buffer_comunicazione.head + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&buffer_comunicazione.mutex);
    sem_post(&buffer_comunicazione.full);
}


// Inizializza il mutex
void inizializza_coda(ComandoCoda *coda) {
    coda->head = coda->tail = NULL;
    pthread_mutex_init(&coda->mutex, NULL);
}


// Distrugge il mutex
void distruggi_coda(ComandoCoda *coda) {
    ComandoNodo *puntatore = coda->head;
    while(puntatore) { 
        ComandoNodo* temp = puntatore; 
        puntatore = puntatore->next; 
        free(temp); }
    pthread_mutex_destroy(&coda->mutex);
}


// Aggiunge un nodo in fondo alla coda
void accoda_comando(ComandoCoda *coda, Messaggio msg) {
    ComandoNodo *nuovo = malloc(sizeof(ComandoNodo));
    if (!nuovo) {
        return;
    }
    nuovo->comando = msg; 
    nuovo->next = NULL;
    pthread_mutex_lock(&coda->mutex);
    if (coda->tail) {
        coda->tail->next = nuovo;
    } 
    else {
        coda->head = nuovo;
    }
    coda->tail = nuovo;
    pthread_mutex_unlock(&coda->mutex);
}


// Rimuove un nodo dalla testa della coda
bool preleva_comando(ComandoCoda *coda, Messaggio *msg) {
    pthread_mutex_lock(&coda->mutex);
    if (!coda->head) {
        pthread_mutex_unlock(&coda->mutex);
        return false;
    }
    ComandoNodo *temp = coda->head;
    *msg = temp->comando;
    coda->head = temp->next;
    if (!coda->head) {
        coda->tail = NULL;
    }
    pthread_mutex_unlock(&coda->mutex);
    free(temp);
    return true;
}


// Main
int main() {
    srand(time(NULL));
    inizializza_buffer();
    inizializza_coda(&coda_proiettili);
    inizializza_coda(&coda_granate);
    pthread_mutex_init(&ncurses_mutex, NULL);

    while (avvia_sessione());

    distruggi_buffer();
    distruggi_coda(&coda_proiettili);
    distruggi_coda(&coda_granate);
    pthread_mutex_destroy(&ncurses_mutex);
    return 0;
}


bool avvia_sessione() {
    pthread_mutex_lock(&ncurses_mutex);
    initscr();
    pthread_mutex_unlock(&ncurses_mutex);

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
            pthread_mutex_lock(&ncurses_mutex);
            endwin();
            pthread_mutex_unlock(&ncurses_mutex);
            return false;
    }
    return true;
}


int mostra_menu() {
    pthread_mutex_lock(&ncurses_mutex);
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
    pthread_mutex_unlock(&ncurses_mutex);
    return getch();
}
