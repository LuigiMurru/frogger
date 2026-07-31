#include "frogger.h"

// Gestore dedicato ai coccodrilli ('C' è la posizione, 'S' la richiesta di sparo)
void processo_coccodrillo(int id, int start_x, int y, int max_x_val, float moltiplicatore_velocita, int pipe_out, int pipe_in) {
    int x = start_x;
    int corsia = id % NUM_CORSIE_FIUME;
    int dx = (corsia % 2 == 0) ? 1 : -1;
    bool flag = FALSE;
    srand(time(NULL) ^ (getpid() << 16));

    while (1) {  //abbiamo provato a far cambiare posizione ai coccodrilli al cambio manche 
                 //ma ogni tanto i coccodrilli si sovrappongono quindi l'abbiamo omessa
        /*
        read(pipe_in, &flag, sizeof(bool)); //controllo se è finita una manche
        if(flag){
            if(corsia != 1){  //aiuta la corsia più in basso a prendere il suo messaggio di reset
                usleep(5000);
            }
            //cambio la posizione ai coccodrilli
            if(id < 9){  //Seconda metà dello schermo
                x = x = (rand()%max_x -LARGHEZZA_COCCODRILLO) + max_x/2 + 1;;
            }
            else {      //Prima metà dello schermo
                x = (rand()% (max_x/2) - LARGHEZZA_COCCODRILLO) + LARGHEZZA_COCCODRILLO;  
            }
            flag = FALSE;
        }
        */
        x += dx;
        if (dx == 1 && x > max_x_val) {
            x = -LARGHEZZA_COCCODRILLO;
        }
        if (dx == -1 && x + LARGHEZZA_COCCODRILLO < 0) {
            x = max_x_val;
        }

        Messaggio msg_pos = {'C', id, x, y, dx};
        write(pipe_out, &msg_pos, sizeof(Messaggio));
        
        if (rand() % 1000 < 50) {   // Rateo di sparo (probabilità del 5%)
            Messaggio msg_sparo = {'S', id, x, y, dx};
            write(pipe_out, &msg_sparo, sizeof(Messaggio));
        }
        usleep(DELAY * (corsia % 4 + 1) * moltiplicatore_velocita);
    }
}


// Gestore dedicato alla rana ('M' è il movimento, 'L' la richiesta di lancio)
void processo_rana(int pipe_stato_in, int pipe_eventi_out) {
    nodelay(stdscr, TRUE);
    fcntl(pipe_stato_in, F_SETFL, fcntl(pipe_stato_in, F_GETFL, 0) | O_NONBLOCK);

    while(1) {
        int ch = getch();
        if (ch != ERR) {
            Messaggio msg;

            if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT || ch == 'q' || ch == 'Q') {
                msg.tipo = 'M';      // M se riceve un comando di movimento
                msg.id = ch;
                write(pipe_eventi_out, &msg, sizeof(Messaggio));
            } 
            else if (ch == ' ') {
                msg.tipo = 'L';      // L se riceve una richiesta di lancio
                msg.id = 0;
                write(pipe_eventi_out, &msg, sizeof(Messaggio));
            }
        }
        usleep(16000); 
    }
}

// Gestore dedicato ai proiettili ('P' è lo sparo effettivo, 'K' quando il proiettile deve essere distrutto, 'U' quando la lista va aggiornata, 'Z' se vanno eliminati tutti)
void processo_gestore_proiettili(int pipe_in, int pipe_out, int max_x_val) {
    NodoProiettile *lista_proiettili_interna = NULL;
    Messaggio msg;
    long prossimo_id = 0;

    fcntl(pipe_in, F_SETFL, fcntl(pipe_in, F_GETFL, 0) | O_NONBLOCK);

    while (1) {
        while (read(pipe_in, &msg, sizeof(Messaggio)) > 0) {
            if (msg.tipo == 'P') {
                NodoProiettile *nuovo = malloc(sizeof(NodoProiettile));
                if (nuovo) {
                    nuovo->id = prossimo_id;
                    prossimo_id += 2; 
                    nuovo->tipo = msg.tipo;
                    nuovo->x = msg.x; 
                    nuovo->y = msg.y; 
                    nuovo->dx = msg.dx;
                    nuovo->da_rimuovere = false; 
                    nuovo->next = lista_proiettili_interna;
                    lista_proiettili_interna = nuovo;
                }
            } 
            else if (msg.tipo == 'K') {
                for (NodoProiettile *p = lista_proiettili_interna; p; p = p->next) {
                    if (p->id == msg.id) { 
                        p->da_rimuovere = true; 
                        break; 
                    }
                }
            }
            else if (msg.tipo == 'Z') {     // A fine manche muoiono naturalmente
                for (NodoProiettile *p = lista_proiettili_interna; p; p = p->next) {
                    p->x = msg.x;
                }
            }
        }

        for (NodoProiettile *p = lista_proiettili_interna; p; p = p->next) {
            p->x += p->dx;
            if (p->x < -1 || p->x > max_x_val) {
                p->da_rimuovere = true;
            }
        }
        
        // Invio degli aggiornamenti sulla pipe condivisa
        Messaggio sync_lista = {'U', -1, 0, 0, 0};          // ID -1 per sync proiettili
        write(pipe_out, &sync_lista, sizeof(Messaggio));
        
        for (NodoProiettile *p = lista_proiettili_interna; p; p = p->next) {
            if (!p->da_rimuovere) {
                Messaggio msg_update = {p->tipo, p->id, p->x, p->y, p->dx};
                write(pipe_out, &msg_update, sizeof(Messaggio));
            }
        }
        
        NodoProiettile **puntatore = &lista_proiettili_interna;
        while (*puntatore) {
            if ((*puntatore)->da_rimuovere) {
                NodoProiettile *temp = *puntatore;
                *puntatore = temp->next; free(temp);
            } 
            else { 
                puntatore = &(*puntatore)->next; 
            }
        }
        usleep(50000);
    }
}

// Gestore dedicato alle granate ('G' è il lancio effettivo, 'K' quando la granata deve essere distrutta, 'U' quando la lista va aggiornata, 'Z' se vanno eliminate tutte)
void processo_gestore_granate(int pipe_in, int pipe_out, int max_x_val) {
    NodoProiettile *lista_granate_interna = NULL;
    Messaggio msg;
    long prossimo_id = 1;

    fcntl(pipe_in, F_SETFL, fcntl(pipe_in, F_GETFL, 0) | O_NONBLOCK);

    while (1) {
        while (read(pipe_in, &msg, sizeof(Messaggio)) > 0) {
            if (msg.tipo == 'G') {
                //Granata sinistra
                NodoProiettile *nuovo_sx = malloc(sizeof(NodoProiettile));
                if (nuovo_sx) {
                    nuovo_sx->id = prossimo_id;
                    prossimo_id += 2;
                    nuovo_sx->tipo = 'G';
                    nuovo_sx->x = msg.x;
                    nuovo_sx->y = msg.y + 1;
                    nuovo_sx->dx = -1;
                    nuovo_sx->da_rimuovere = false;
                    nuovo_sx->next = lista_granate_interna;
                    lista_granate_interna = nuovo_sx;
                }
                // Granata destra
                NodoProiettile *nuovo_dx = malloc(sizeof(NodoProiettile));
                if (nuovo_dx) {
                    nuovo_dx->id = prossimo_id;
                    prossimo_id += 2;
                    nuovo_dx->tipo = 'G';
                    nuovo_dx->x = msg.x + 1;
                    nuovo_dx->y = msg.y + 1;
                    nuovo_dx->dx = 1;
                    nuovo_dx->da_rimuovere = false;
                    nuovo_dx->next = lista_granate_interna;
                    lista_granate_interna = nuovo_dx;
                }
            } 
            else if (msg.tipo == 'K') {
                for (NodoProiettile *p = lista_granate_interna; p; p = p->next) {
                    if (p->id == msg.id) { 
                        p->da_rimuovere = true; 
                        break; 
                    }
                }
            }
            else if (msg.tipo == 'Z') {     //a fine manche faccio morire naturalmente le granate
                for (NodoProiettile *p = lista_granate_interna; p; p = p->next) {
                    p->x = msg.x;
                }
            }
        }

        for (NodoProiettile *p = lista_granate_interna; p; p = p->next) {
            p->x += p->dx * 2;
            if (p->x < -1 || p->x > max_x_val) {
                p->da_rimuovere = true;
            }
        }
        
        // Invio degli aggiornamenti sulla pipe condivisa
        Messaggio sync_lista = {'U', -2, 0, 0, 0};          // ID -2 per sync granate
        write(pipe_out, &sync_lista, sizeof(Messaggio));

        for (NodoProiettile *p = lista_granate_interna; p; p = p->next) {
            if (!p->da_rimuovere) {
                Messaggio msg_update = {p->tipo, p->id, p->x, p->y, p->dx};
                write(pipe_out, &msg_update, sizeof(Messaggio));
            }
        }

        NodoProiettile **puntatore = &lista_granate_interna;
        while (*puntatore) {
            if ((*puntatore)->da_rimuovere) {
                NodoProiettile *temp = *puntatore;
                *puntatore = temp->next; 
                free(temp);
            } 
            else { 
                puntatore = &(*puntatore)->next; 
            }
        }
        usleep(50000);
    }
}
