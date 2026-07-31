#include "frogger.h"

void disegna_interfaccia() {
    erase();
    int prima_riga_fiume = 4;
    int zona_fiume_y_fine = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);


    // Marciapiede superiore
    attron(COLOR_PAIR(COPPIA_COLORE_MARCIAPIEDE));
    for (int y=1; y < prima_riga_fiume; y++) {
        for (int i = 0; i < max_x; i++) {
            mvaddch(y, i, ' ');
        }
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_MARCIAPIEDE));


    // Marciapiede inferiore
    attron(COLOR_PAIR(COPPIA_COLORE_MARCIAPIEDE));
    for(int y=zona_fiume_y_fine; y < zona_fiume_y_fine + 3; y++) {
        for(int i = 0; i < max_x; i++) {
            mvaddch(y, i, ' ');
        }
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_MARCIAPIEDE));


    // Animazione onde d'acqua
    animazione_acqua_timer--;
    if (animazione_acqua_timer <= 0) {
        animazione_acqua_timer = 10;            // Ogni 10 frame, genera un nuovo pattern di onde e lo salva nel buffer
        for (int y = prima_riga_fiume; y < zona_fiume_y_fine; y++) {
            for (int x = 0; x < max_x; x++) {
                if (y < 64 && x < 256) {        // Controllo di sicurezza per non scrivere fuori dal buffer
                    buffer_acqua[y][x] = (rand() % 20 < 1) ? '~' : '-';
                }
            }
        }
    }
    attron(COLOR_PAIR(COPPIA_COLORE_ACQUA));
    for (int y = prima_riga_fiume; y < zona_fiume_y_fine; y++) {
        for (int x = 0; x < max_x; x++) {
            if (y < 64 && x < 256) {
                mvaddch(y, x, buffer_acqua[y][x]);      // Disegna l'acqua usando il pattern salvato nel buffer
            }
        }
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_ACQUA));


    // Tane
    attron(COLOR_PAIR(COPPIA_COLORE_TANA));
    for (int i = 0; i < NUM_TANE; i++) {
        mvprintw(1, tane_x[i] - 5, "|         |");
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_TANA));


    // Coccodrilli
    for (int i = 0; i < NUM_COCCODRILLI; i++) {
        attron(COLOR_PAIR(COPPIA_COLORE_COCCODRILLO));
        int corsia = i % NUM_CORSIE_FIUME;
        int dx = (corsia % 2 == 0) ? 1 : -1;
        const char** sprite = (dx == 1) ?                           // Seleziona lo sprite corretto per i coccodrilli
            (cocco_sparo_timer[i] > 0 ? SPRITE_COCCO_DESTRA_SPARO : SPRITE_COCCO_DESTRA) :
            (cocco_sparo_timer[i] > 0 ? SPRITE_COCCO_SINISTRA_SPARO : SPRITE_COCCO_SINISTRA);
        for (int r = 0; r < ALTEZZA_COCCODRILLO; r++) {
            mvprintw(cocco_y[i] + r, cocco_x[i], "%s", sprite[r]);
        }
        attroff(COLOR_PAIR(COPPIA_COLORE_COCCODRILLO));
    }


    // Proiettili e granate
    for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
        attron(COLOR_PAIR(p->tipo == 'G' ? COPPIA_COLORE_GRANATA : COPPIA_COLORE_PROIETTILE));
        mvaddch(p->y, p->x, (p->tipo == 'G') ? 'o' : 'x');
        attroff(COLOR_PAIR(p->tipo == 'G' ? COPPIA_COLORE_GRANATA : COPPIA_COLORE_PROIETTILE));
    }


    // Rana
    attron(COLOR_PAIR(COPPIA_COLORE_RANA));
    for (int i = 0; i < ALTEZZA_RANA; i++) {
        mvprintw(rana_y + i, rana_x, "%s", SPRITE_RANA[i]);
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_RANA));


    // Rana in tana
    attron(COLOR_PAIR(COPPIA_COLORE_RANA));
    for (int i = 0; i < NUM_TANE; i++) {
        if (tane_occupate[i]) {
            mvprintw(1, tane_x[i] - 5, "|----O----|");
        }
    }
    attroff(COLOR_PAIR(COPPIA_COLORE_RANA));


    // Informazioni a schermo
    mvprintw(0, (max_x + 45)/2, "Vite: %d", vite);
    mvprintw(0, (max_x + 5)/2, "Punteggio: %d", punteggio);
    mvprintw(0, (max_x - 35)/2, "Secondi: %d", timer);
    mvprintw(0, (max_x - 75)/2, "Q: Pausa");
    refresh();
}


// Scelta difficoltà
void difficolta() {
    bool indietro = false;
    while(!indietro){
        pthread_mutex_lock(&ncurses_mutex);
        clear();
        mvprintw(max_y/2 - 4, (max_x - 10)/2, "Difficoltà");
        mvprintw(max_y/2, (max_x - 62)/2, "1 -> FACILE - 10 vite, 90 secondi, Velocità coccodrilli: lenta");
        mvprintw(max_y/2 + 2, (max_x - 62)/2, "2 -> MEDIO - 5 vite, 60 secondi, Velocità coccodrilli: intermedia");
        mvprintw(max_y/2 + 4, (max_x - 62)/2, "3 -> DIFFICILE - 3 vite, 45 secondi, Velocità coccodrilli: alta");
        mvprintw(max_y/2 + 6, (max_x - 62)/2, "4 -> Indietro");
        refresh();
        int scelta = getch();
        pthread_mutex_unlock(&ncurses_mutex);
        switch (scelta){
            case '1': 
                impostazioni_correnti = (ImpostazioniDifficolta){10, 90, 1.0f}; 
                gioca(); 
                indietro = true; 
                break;
            case '2': 
                impostazioni_correnti = (ImpostazioniDifficolta){5, 60, 0.8f}; 
                gioca(); 
                indietro = true; 
                break;
            case '3': 
                impostazioni_correnti = (ImpostazioniDifficolta){3, 45, 0.6f}; 
                gioca(); 
                indietro = true; 
                break;
            case '4': 
                indietro = true; 
                break;
        }
    }
}


// Istruzioni di gioco
void istruzioni() {
    pthread_mutex_lock(&ncurses_mutex);
    clear();
    mvprintw(max_y/2 - 4, (max_x - 10)/2, "Istruzioni");
    mvprintw(max_y/2, (max_x - 64)/2, "-> OBIETTIVO: portare la rana in salvo nelle cinque tane in alto");
    mvprintw(max_y/2 + 2, (max_x - 64)/2, "-> MOVIMENTO: usa le frecce direzionali");
    mvprintw(max_y/2 + 4, (max_x - 64)/2, "-> SPARA GRANATE: barra spaziatrice (max. 1 al secondo)");
    mvprintw(max_y/2 + 6, (max_x - 64)/2, "-> MENU DI PAUSA: tasto Q");
    mvprintw(max_y/2 + 8, (max_x - 64)/2, "-> PUNTEGGIO: +50 per tana chiusa;");
    mvprintw(max_y/2 + 10, (max_x - 37)/2, "-10 per morte;");
    mvprintw(max_y/2 + 12, (max_x - 37)/2, "+ punti per il tempo rimanente.");
    mvprintw(max_y/2 + 16, (max_x - 47)/2, "Premi un tasto qualsiasi per tornare al menu...");
    refresh();
    getch();
    pthread_mutex_unlock(&ncurses_mutex);
}


// Crediti
void crediti() {
    pthread_mutex_lock(&ncurses_mutex);
    clear();
    mvprintw(max_y/2 - 4, (max_x - 21)/2, "Frogger: Resurrection");
    mvprintw(max_y/2, (max_x - 46)/2, "Progetto a cura di Daniele Dessì e Luigi Murru");
    mvprintw(max_y/2 + 4, (max_x - 47)/2, "Premi un tasto qualsiasi per tornare al menu...");
    refresh();
    getch();
    pthread_mutex_unlock(&ncurses_mutex);
}


// Menu di pausa
void pausa() {
    pthread_mutex_lock(&ncurses_mutex);
    clear();
    mvprintw(max_y/2 - 4, (max_x - 5)/2, "Pausa");
    mvprintw(max_y/2, (max_x - 32)/2, "Premi Q per riprendere a giocare");
    refresh();
    pthread_mutex_unlock(&ncurses_mutex);
}


int leggi_record() {
    FILE *file = fopen(NOME_FILE_RECORD, "r");
    int record = 0;

    if (file == NULL) {
        return 0;
    }
    fscanf(file, "%d", &record);
    fclose(file);

    return record;
}


int scrivi_record(int nuovo_record) {
    FILE *file = fopen(NOME_FILE_RECORD, "w");

    if (file == NULL) {
        return -1;
    }
    fprintf(file, "Record personale: %d", nuovo_record);
    fclose(file);
    return 0;
}


// Schermata di fine partita
void fine_partita() {
    pthread_mutex_lock(&ncurses_mutex);
    clear();

    if (tane_raggiunte == NUM_TANE) {
        mvprintw(max_y/2 - 4, (max_x - 10)/2, "Hai vinto!");
    }
    else {
        mvprintw(max_y/2 - 4, (max_x - 10)/2, "Hai perso!");
    }
    mvprintw(max_y/2, (max_x - 20)/2, "Punteggio Finale: %d", punteggio);

    int record_precedente = leggi_record();

    if (punteggio > record_precedente) {
        int risultato_salvataggio = scrivi_record(punteggio);
        if (risultato_salvataggio == 0) {
            mvprintw(max_y/2 + 2, (max_x - 14)/2, "Nuovo record: %d", punteggio);
        }
    }
    else {
        mvprintw(max_y/2 + 2, (max_x - 21)/2, "Record da battere: %d", record_precedente);
    }

    mvprintw(max_y/2 + 6, (max_x - 47)/2, "Premi un tasto qualsiasi per tornare al menu...");
    noecho();
    curs_set(FALSE);
    refresh();

    nodelay(stdscr, FALSE);
    getch();
    pthread_mutex_unlock(&ncurses_mutex);
}