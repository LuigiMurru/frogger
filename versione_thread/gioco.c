#include "frogger.h"

void start_rana(){
    int prima_riga_fiume = 4;
    int y_marciapiede_basso = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
    rana_x = max_x / 2;
    rana_y = y_marciapiede_basso;
    id_cocco_cavalcato = -1;
}

// Posizione iniziale della rana
void reset_rana() {
    start_rana();
    accoda_comando(&coda_granate, (Messaggio){'Z', 0, 0, 0, 0});
    accoda_comando(&coda_proiettili, (Messaggio){'Z', 0, 0, 0, 0});
    for (NodoProiettile *p = lista_proiettili; p != NULL; p = p->next) {
        p->da_rimuovere = true;
    }
    for (int i = 0; i < NUM_CORSIE_FIUME; i++) {
        cocco_x[i] = rand() % max_x;
        cocco_x[i + NUM_CORSIE_FIUME] = (cocco_x[i] + max_x / 2) % max_x;
    }
}


// Gestione del gioco
void gioca() {
    pthread_mutex_lock(&ncurses_mutex);
    nodelay(stdscr, TRUE);
    pthread_mutex_unlock(&ncurses_mutex);

    // Inizializzazione variabili di gioco
    vite = impostazioni_correnti.vite; 
    timer = impostazioni_correnti.tempo_gioco;
    punteggio = 0; 
    partita_finita = false; 
    partita_in_corso = true;
    bool in_pausa = false; 
    tane_raggiunte = 0;
    for (int i = 0; i < NUM_TANE; i++) {
        tane_occupate[i] = 0;
    }

    int prima_riga_fiume = 4;
    for (int i = 0; i < NUM_CORSIE_FIUME; i++) {
        int y_corsia = prima_riga_fiume + (i * ALTEZZA_COCCODRILLO);
        cocco_y[i] = y_corsia; cocco_y[i + NUM_CORSIE_FIUME] = y_corsia;
        cocco_x[i] = rand() % max_x; cocco_x[i + NUM_CORSIE_FIUME] = (cocco_x[i] + max_x / 2) % max_x;
    }

    start_rana();

    for (int i = 0; i < NUM_TANE; i++) {
        tane_x[i] = (max_x / NUM_TANE) * i + (max_x / (NUM_TANE * 2));
    }

    // Creazione thread coccodrilli
    pthread_t thread_coccodrilli[NUM_COCCODRILLI];
    for(int i = 0; i < NUM_COCCODRILLI; i++) {
        Coccodrillo *args = malloc(sizeof(Coccodrillo));
        args->id = i; 
        args->start_x = cocco_x[i]; 
        args->y = cocco_y[i];
        args->moltiplicatore_velocita = impostazioni_correnti.moltiplicatore_velocita;
        pthread_create(&thread_coccodrilli[i], NULL, thread_coccodrillo, args);
    }
    
    // Creazione thread rana
    pthread_t tid_rana;
    pthread_create(&tid_rana, NULL, thread_rana, NULL);

    // Creazione thread proiettili e granate
    pthread_t tid_gestore_proiettili, tid_gestore_granate;
    GranateProiettili *p_args = malloc(sizeof(GranateProiettili)); 
    p_args->max_x_val = max_x;
    GranateProiettili *g_args = malloc(sizeof(GranateProiettili)); 
    g_args->max_x_val = max_x;
    pthread_create(&tid_gestore_proiettili, NULL, thread_gestore_proiettili, p_args);
    pthread_create(&tid_gestore_granate, NULL, thread_gestore_granate, g_args);

    time_t ultimo_aggiornamento_timer = time(NULL);

    while(!partita_finita) {
        while (sem_trywait(&buffer_comunicazione.full) == 0) {
            pthread_mutex_lock(&buffer_comunicazione.mutex);
            Messaggio msg = buffer_comunicazione.buffer[buffer_comunicazione.tail];
            buffer_comunicazione.tail = (buffer_comunicazione.tail + 1) % BUFFER_SIZE;
            pthread_mutex_unlock(&buffer_comunicazione.mutex);
            sem_post(&buffer_comunicazione.empty);

            switch(msg.tipo) {
                case 'C':                       // C: posizione coccodrillo
                    if (msg.id >= 0 && msg.id < NUM_COCCODRILLI) {
                        cocco_x[msg.id] = msg.x;
                    }
                    break;
                case 'S':
                    if (msg.id >= 0 && msg.id < NUM_COCCODRILLI) {
                        cocco_sparo_timer[msg.id] = 8;
                        int proj_x = (msg.dx == 1) ? msg.x + LARGHEZZA_COCCODRILLO : msg.x - 1;
                        accoda_comando(&coda_proiettili, (Messaggio){'P', 0, proj_x, msg.y + 1, msg.dx});
                    } 
                    break;
                case 'M':                        // S: richiesta di sparo proiettile
                    if (msg.id == 'q' || msg.id == 'Q') {
                        in_pausa = !in_pausa;
                    }
                    else if (!in_pausa) {
                        int ch = msg.id;
                        int y_partenza = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
                        if (ch == KEY_UP && rana_y > 1) {
                            rana_y = (rana_y - 2 < 1) ? 1 : rana_y - 2;                     // Movimento su
                        }
                        else if (ch == KEY_DOWN && rana_y < y_partenza) {
                            rana_y = (rana_y + 2 > y_partenza) ? y_partenza : rana_y + 2;   // La rana non può scendere sotto il punto di partenza
                        }
                        else if (ch == KEY_LEFT) {                                          // Movimento della rana a sinistra se è su un coccodrillo
                            (id_cocco_cavalcato != -1) ? offset_rana-- : rana_x--;
                        }
                        else if (ch == KEY_RIGHT) {                                         // Movimento della rana a destra se è su un coccodrillo
                            (id_cocco_cavalcato != -1) ? offset_rana++ : rana_x++;
                        }
                    }
                    break;
                case 'L':                   // L: richiesta di lancio granata                               
                    if (!in_pausa) {
                        time_t t = time(NULL);
                        if (t - ultimo_lancio_granata_t >= 1) {
                            accoda_comando(&coda_granate, (Messaggio){'G', 0, rana_x, rana_y, 0});
                            ultimo_lancio_granata_t = t;
                        }
                    } 
                    break;
                case 'U':                   // Update: indica che i proiettili o le granate vanno segnati come da rimuovere
                    aggiorna_proiettile(msg); 
                    break;
                case 'D':                   // Distruzione: indica che i proiettili o le granate sono usciti dallo schermo
                    for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
                        if (p->id == msg.id) { 
                            p->da_rimuovere = true; 
                            break; 
                        }
                    } 
                    break;
            }
        }

        // Logica di pausa
        if (in_pausa) { 
            pausa(); 
            usleep(100000); 
            continue; 
        }

        // Timer per cambio sprite coccodrillo (gestito in interfaccia.c)
        for (int i = 0; i < NUM_COCCODRILLI; i++) { 
            if (cocco_sparo_timer[i] > 0) {
                cocco_sparo_timer[i]--;
            }
        }

        time_t tempo_corrente = time(NULL);
        if ((tempo_corrente - ultimo_aggiornamento_timer) >= 1) {
            timer--;
            ultimo_aggiornamento_timer = tempo_corrente;
            if (timer <= 0) { 
                punteggio = (punteggio >= 10) ? punteggio - 10 : 0; vite--; 
                reset_rana(); timer = impostazioni_correnti.tempo_gioco; 
            }
        }

        if (id_cocco_cavalcato != -1) {         // La rana segue i movimenti del coccodrillo
            rana_x = cocco_x[id_cocco_cavalcato] + offset_rana;
        }
        if (rana_x < 0) {                       // La rana non può uscire dallo schermo
            rana_x = 0;
        } 
        if (rana_x > max_x - LARGHEZZA_RANA) {
            rana_x = max_x - LARGHEZZA_RANA;
        }

        // Controllo della rana se è su un coccodrillo
        int nuovo_id_cocco_cavalcato = -1;
        for (int i = 0; i < NUM_COCCODRILLI; i++) {
            if (collisione_rettangoli(rana_x, rana_y, LARGHEZZA_RANA, ALTEZZA_RANA, cocco_x[i], cocco_y[i], LARGHEZZA_COCCODRILLO, ALTEZZA_COCCODRILLO)) {
                nuovo_id_cocco_cavalcato = i; 
                break;
            }
        }
        // La rana sale su un coccodrillo per la prima volta
        if (nuovo_id_cocco_cavalcato != -1 && id_cocco_cavalcato == -1) { 
            id_cocco_cavalcato = nuovo_id_cocco_cavalcato; 
            offset_rana = rana_x - cocco_x[id_cocco_cavalcato]; 
        }
        // La rana scende dal coccodrillo
        else if (nuovo_id_cocco_cavalcato == -1 && id_cocco_cavalcato != -1) { 
            id_cocco_cavalcato = -1; 
        }
        // La rana passa da un coccodrillo a un altro
        else if (nuovo_id_cocco_cavalcato != -1 && nuovo_id_cocco_cavalcato != id_cocco_cavalcato) { 
            id_cocco_cavalcato = nuovo_id_cocco_cavalcato; 
            offset_rana = rana_x - cocco_x[id_cocco_cavalcato]; 
        }

        // Controllo caduta in acqua della rana
        int zona_fiume_y_fine = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
        if ((rana_y >= prima_riga_fiume && rana_y < zona_fiume_y_fine) && id_cocco_cavalcato == -1) { 
            punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
            vite--; 
            reset_rana(); 
        }

        // Controllo della rana in tana
        if (rana_y == 1) {
            bool colpita_tana = false;
            for (int i = 0; i < NUM_TANE; i++) {
                if (rana_x >= tane_x[i] - 3 && rana_x <= tane_x[i] + 3) {
                    colpita_tana = true;
                    if (tane_occupate[i] == 0) {                    // Se la tana era vuota, allora viene occupata
                        tane_occupate[i] = 1; 
                        tane_raggiunte++; 
                        punteggio += 50 + timer;                    // Il giocatore guadagna 50 punti per la tana, più il punteggio del tempo rimanente
                        timer = impostazioni_correnti.tempo_gioco; 
                        if (tane_raggiunte == NUM_TANE) {           // Se tutte le tane son state occupate, il giocatore ha vinto
                            partita_finita = true;
                        } 
                    } 
                    else {                                          // Se la tana era già stata occupata, la rana perde una vita
                        punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
                        vite--; 
                    }
                    reset_rana();
                    break;
                }
            }
            if (!colpita_tana) { 
                punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
                vite--; 
                reset_rana(); 
            }
        }
        gestione_collisioni();
        if (vite <= 0) {
            partita_finita = true;
        }

        rimuovi_p_g();
        
        pthread_mutex_lock(&ncurses_mutex);
        disegna_interfaccia();
        pthread_mutex_unlock(&ncurses_mutex);
        
        usleep(16000);
    }
    
    partita_in_corso = false;

    // Pulisce tutti i proiettili rimasti dopo la fine del gioco
    for(NodoProiettile* p = lista_proiettili; p != NULL; p = p->next) {
        p->da_rimuovere = true;
    }

    rimuovi_p_g();
    fine_partita();
    
    pthread_mutex_lock(&ncurses_mutex);
    nodelay(stdscr, FALSE);
    getch();
    endwin();
    pthread_mutex_unlock(&ncurses_mutex);
    
    pthread_join(tid_rana, NULL);
    for (int i = 0; i < NUM_COCCODRILLI; i++) {
        pthread_join(thread_coccodrilli[i], NULL);
    }
    pthread_join(tid_gestore_proiettili, NULL);
    pthread_join(tid_gestore_granate, NULL);
}