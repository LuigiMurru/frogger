#include "frogger.h"

void start_rana(){
    int prima_riga_fiume = 4;
    int y_marciapiede_basso = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
    rana_x = max_x / 2;
    rana_y = y_marciapiede_basso;
    id_cocco_cavalcato = -1;
}


// Posizione iniziale della rana
void reset_rana(int pipe_a_cocco, int pipe_produttori_a_consumatore, int pipe_a_gestore_proiettili, int pipe_a_gestore_granate) {
    start_rana();
    
    /*  Parte omessa, motivazione in processi.c riga 11
    // Avviso della fine della manche
    for(int i = 0; i<16; i++){
        write(pipe_a_cocco, &(bool){TRUE}, sizeof(bool));
    }
    */

    write(pipe_a_gestore_granate, &(Messaggio){'Z', -2, -5, 0, 0}, sizeof(Messaggio));
    write(pipe_a_gestore_proiettili, &(Messaggio){'Z', -1, -5, 0, 0}, sizeof(Messaggio));
}


// Gestione del gioco
void gioca() {
    nodelay(stdscr, TRUE);

    if (pipe(pipe_produttori_a_consumatore) == -1 || pipe(pipe_a_rana) == -1 ||
        pipe(pipe_a_cocco) == -1 ||
        pipe(pipe_a_gestore_proiettili) == -1 || pipe(pipe_a_gestore_granate) == -1) {
        perror("pipe"); return;
    }
    
    // Pipe di lettura non bloccante
    fcntl(pipe_produttori_a_consumatore[0], F_SETFL, fcntl(pipe_produttori_a_consumatore[0], F_GETFL, 0) | O_NONBLOCK);
    fcntl(pipe_a_cocco[0], F_SETFL, fcntl(pipe_a_cocco[0], F_GETFL, 0) | O_NONBLOCK);

    // Pipe di scrittura non bloccanti
    fcntl(pipe_a_rana[1], F_SETFL, fcntl(pipe_a_rana[1], F_GETFL, 0) | O_NONBLOCK);
    fcntl(pipe_a_gestore_proiettili[1], F_SETFL, fcntl(pipe_a_gestore_proiettili[1], F_GETFL, 0) | O_NONBLOCK);
    fcntl(pipe_a_gestore_granate[1], F_SETFL, fcntl(pipe_a_gestore_granate[1], F_GETFL, 0) | O_NONBLOCK);

    // Inizializzazione variabili di gioco
    vite = impostazioni_correnti.vite; 
    timer = impostazioni_correnti.tempo_gioco;
    punteggio = 0; 
    partita_finita = false; 
    bool in_pausa = false; 
    tane_raggiunte = 0;
    for (int i = 0; i < NUM_TANE; i++) {
        tane_occupate[i] = 0;
    }

    int prima_riga_fiume = 4;
    for (int i = 0; i < NUM_CORSIE_FIUME; i++) {
        int y_corsia = prima_riga_fiume + (i * ALTEZZA_COCCODRILLO);
        cocco_y[i] = y_corsia;                                                          // Coordinata del primo coccodrillo lungo l'asse y
        cocco_y[i + NUM_CORSIE_FIUME] = y_corsia;                                       // Coordinata del secondo coccodrillo lungo l'asse y, ma stesso asse x del primo
        cocco_x[i] = rand() % max_x;                                                    // Posizione casuale del primo coccodrillo
        vecchio_cocco_x[i] = cocco_x[i];
        cocco_x[i + NUM_CORSIE_FIUME] = (cocco_x[i] + max_x / 2) % max_x;               // Il secondo coccodrillo viene sfasato di mezza larghezza dello schermo
        vecchio_cocco_x[i + NUM_CORSIE_FIUME] = cocco_x[i + NUM_CORSIE_FIUME];
    }
    
    start_rana();
    
    for (int i = 0; i < NUM_TANE; i++) {
        tane_x[i] = (max_x / NUM_TANE) * i + (max_x / (NUM_TANE * 2));
    }

    // Creazione processi coccodrilli
    pid_t pid_coccodrilli[NUM_COCCODRILLI];
    for(int i = 0; i < NUM_COCCODRILLI; i++) {                  // Processo separato per ogni coccodrillo
        if((pid_coccodrilli[i] = fork()) == 0) {
            processo_coccodrillo(i, cocco_x[i], cocco_y[i], max_x, impostazioni_correnti.moltiplicatore_velocita, pipe_produttori_a_consumatore[1], pipe_a_cocco[0]);
            exit(0);
        }
    }

    // Creazione del gestore proiettili
    pid_t pid_gestore_proiettili;
    if ((pid_gestore_proiettili = fork()) == 0) {
        processo_gestore_proiettili(pipe_a_gestore_proiettili[0], pipe_produttori_a_consumatore[1], max_x);
        exit(0);
    }

    // Creazione del gestore granate
    pid_t pid_gestore_granate;
    if ((pid_gestore_granate = fork()) == 0) {
        processo_gestore_granate(pipe_a_gestore_granate[0], pipe_produttori_a_consumatore[1], max_x);
        exit(0);
    }

    // Creazione processo rana
    pid_t pid_rana;
    if ((pid_rana = fork()) == 0) {
        processo_rana(pipe_a_rana[0], pipe_produttori_a_consumatore[1]);
        exit(0);
    }

    // Chiusura estremità pipe non usate
    //close(pipe_produttori_a_consumatore[1]); 
    close(pipe_a_rana[0]);
    close(pipe_a_gestore_proiettili[0]); 
    close(pipe_a_gestore_granate[0]);
    close(pipe_a_cocco[0]);

    Messaggio msg;
    time_t ultimo_aggiornamento_timer = time(NULL);
    Comando comandi_da_inviare[NUM_COCCODRILLI + 2];
    int num_comandi = 0;

    while(!partita_finita) {
        num_comandi = 0;
        bool sync_proiettili_avvenuto = false;
        bool sync_granate_avvenuto = false;
        while (read(pipe_produttori_a_consumatore[0], &msg, sizeof(Messaggio)) > 0) {
             switch(msg.tipo) {
                case 'C':                   // C: posizione coccodrillo
                    if (msg.id >= 0 && msg.id < NUM_COCCODRILLI) {
                        cocco_x[msg.id] = msg.x;
                    } 
                    break;
                case 'S':                   // S: richiesta di sparo proiettile
                    if (msg.id >= 0 && msg.id < NUM_COCCODRILLI) {
                        cocco_sparo_timer[msg.id] = 8;
                        comandi_da_inviare[num_comandi++] = (Comando){'P', (msg.dx == 1) ? msg.x + LARGHEZZA_COCCODRILLO : msg.x - 1, msg.y + 1, msg.dx};
                    } 
                    break;
                case 'M':                   // M: movimento o pausa da parte della rana
                    if (msg.id == 'q' || msg.id == 'Q') { 
                        in_pausa = !in_pausa; 
                    } 
                    else if (!in_pausa) {
                        int ch = msg.id; 
                        int y_partenza = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
                        if (ch == KEY_UP && rana_y > 1) { 
                            rana_y -= 2;                                        // Movimento su
                            if (rana_y < 1) {
                                rana_y = 1;
                            } 
                        } 
                        else if (ch == KEY_DOWN && rana_y < y_partenza) {       // La rana non può scendere sotto il punto di partenza
                            rana_y += 2;                
                            if (rana_y > y_partenza) {
                                rana_y = y_partenza;
                            } 
                        } 
                        else if (ch == KEY_LEFT) { 
                            if (id_cocco_cavalcato != -1) {                     // Movimento della rana a sinistra se è su un coccodrillo
                                offset_rana--;
                            } 
                            else {
                                rana_x--;
                            }
                        } 
                        else if (ch == KEY_RIGHT) {   
                            if(id_cocco_cavalcato != -1) {                      // Movimento della rana a destra se è su un coccodrillo
                                offset_rana++;
                            } 
                            else {
                                rana_x++;
                            }
                        }
                    } 
                    break;
                case 'L':           // L: richiesta di lancio granata
                    if (!in_pausa) {
                        time_t tempo_corrente_granata = time(NULL);
                        if (tempo_corrente_granata - ultimo_lancio_granata_t >= 1) {
                            comandi_da_inviare[num_comandi++] = (Comando){'G', rana_x, rana_y, 0};
                            ultimo_lancio_granata_t = tempo_corrente_granata;
                        }
                    } 
                    break;
                case 'U':                   // Update: indica che i proiettili o le granate vanno segnati come da rimuovere  
                    if (msg.id == -1 && !sync_proiettili_avvenuto) {
                        for (NodoProiettile *p = lista_proiettili; p; p = p->next) { 
                            if (p->tipo == 'P') {
                                p->da_rimuovere = true; 
                            }
                        }
                        sync_proiettili_avvenuto = true;
                    } 
                    else if (msg.id == -2 && !sync_granate_avvenuto) {
                        for (NodoProiettile *p = lista_proiettili; p; p = p->next) { 
                            if (p->tipo == 'G') {
                                p->da_rimuovere = true; 
                            }
                        }
                        sync_granate_avvenuto = true;
                    } 
                    break;
                default: 
                    aggiorna_proiettile(msg); 
                    break;
            }
        }

        // Vengono smistati gli oggetti da creare (P se sono proiettili, G se sono granate)
        for (int i = 0; i < num_comandi; i++) {
            Messaggio nuovo_comando = {comandi_da_inviare[i].tipo, 0, comandi_da_inviare[i].x, comandi_da_inviare[i].y, comandi_da_inviare[i].dx};
            
            if (nuovo_comando.tipo == 'P') {
                write(pipe_a_gestore_proiettili[1], &nuovo_comando, sizeof(Messaggio));
            } 
            else if (nuovo_comando.tipo == 'G') {
                write(pipe_a_gestore_granate[1], &nuovo_comando, sizeof(Messaggio));
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

        // Rimozione proiettili e granate contrassegnati come da rimuovere
        if (sync_proiettili_avvenuto || sync_granate_avvenuto) {
            rimuovi_p_g();
        }
        
        time_t tempo_corrente = time(NULL);
        if ((tempo_corrente - ultimo_aggiornamento_timer) >= 1) {
            timer--;
            ultimo_aggiornamento_timer = tempo_corrente;
            if (timer <= 0) { 
                punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
                vite--; 
                reset_rana(pipe_a_cocco[1], pipe_produttori_a_consumatore[1], pipe_a_gestore_proiettili[1], pipe_a_gestore_granate[1]);
                timer = impostazioni_correnti.tempo_gioco; 
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
        else if (nuovo_id_cocco_cavalcato != -1 && id_cocco_cavalcato != -1 && nuovo_id_cocco_cavalcato != id_cocco_cavalcato) { 
            id_cocco_cavalcato = nuovo_id_cocco_cavalcato; 
            offset_rana = rana_x - cocco_x[id_cocco_cavalcato]; 
        }

        // Controllo caduta in acqua della rana
        int zona_fiume_y_fine = prima_riga_fiume + (NUM_CORSIE_FIUME * ALTEZZA_COCCODRILLO);
        bool in_zona_fiume = (rana_y >= prima_riga_fiume && rana_y < zona_fiume_y_fine);
        if (in_zona_fiume && id_cocco_cavalcato == -1) { 
            punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
            vite--; 
            reset_rana(pipe_a_cocco[1], pipe_produttori_a_consumatore[1], pipe_a_gestore_proiettili[1], pipe_a_gestore_granate[1]);
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
                            partita_finita = 1;
                        }
                    } 
                    else {                      // Se la tana era già stata occupata, la rana perde una vita
                        punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
                        vite--; 
                    }
                    reset_rana(pipe_a_cocco[1], pipe_produttori_a_consumatore[1], pipe_a_gestore_proiettili[1], pipe_a_gestore_granate[1]);
                    break;
                }
            }
            if (!colpita_tana) {            // Se la rana tocca un muro fra le tane, perde una vita
                punteggio = (punteggio >= 10) ? punteggio - 10 : 0; 
                vite--; 
                reset_rana(pipe_a_cocco[1], pipe_produttori_a_consumatore[1], pipe_a_gestore_proiettili[1], pipe_a_gestore_granate[1]);
            }
        }
        gestione_collisioni();

        if (vite <= 0) {
            partita_finita = 1;
        }

        Messaggio msg_stato_rana = {'R', 0, rana_x, rana_y, 0};     // Sincronizzazione rana
        write(pipe_a_rana[1], &msg_stato_rana, sizeof(Messaggio));
        disegna_interfaccia();          // Aggiornamento interfaccia di gioco
        usleep(16000);                  // Controllo velocità di gioco (~60 FPS)
    }

    fine_partita();
    getch();
    
    // Terminazione processi 
    kill(pid_rana, SIGTERM); 
    kill(pid_gestore_proiettili, SIGTERM); 
    kill(pid_gestore_granate, SIGTERM);
    for (int i = 0; i < NUM_COCCODRILLI; i++) { 
        kill(pid_coccodrilli[i], SIGTERM); 
    }
    while (wait(NULL) > 0);
    close(pipe_produttori_a_consumatore[0]); 
    close(pipe_a_rana[1]);
    close(pipe_a_gestore_proiettili[1]); 
    close(pipe_a_gestore_granate[1]);
    
    endwin();
}