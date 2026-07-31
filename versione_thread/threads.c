#include "frogger.h"

// Gestore dedicato ai coccodrilli ('C' è la posizione, 'S' la richiesta di sparo)
void* thread_coccodrillo(void *args) {
    Coccodrillo *c_args = (Coccodrillo*)args; 
    int id = c_args->id; 
    int y = c_args->y; 
    float moltiplicatore = c_args->moltiplicatore_velocita; 
    free(args);
    int corsia = id % NUM_CORSIE_FIUME;
    int dx = (corsia % 2 == 0) ? 1 : -1;
    srand(time(NULL) ^ (long)pthread_self());

    while (partita_in_corso) {
        int x = cocco_x[id] + dx;
        if (dx == 1 && x > max_x) {
            x = -LARGHEZZA_COCCODRILLO;
        }
        if (dx == -1 && x + LARGHEZZA_COCCODRILLO < 0) {
            x = max_x;
        }

        produci((Messaggio){'C', id, x, y, dx});
        if (rand() % 1000 < 50) {
            produci((Messaggio){'S', id, x, y, dx});
        }
        usleep(DELAY * (corsia % 4 + 1) * moltiplicatore);
    }
    return NULL;
}


// Gestore dedicato alla rana ('M' è il movimento, 'L' la richiesta di lancio)
void* thread_rana(void *args) {
    while(partita_in_corso) {
        pthread_mutex_lock(&ncurses_mutex);
        int ch = getch();
        pthread_mutex_unlock(&ncurses_mutex);

        if (ch != ERR) {
            if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT || ch == 'q' || ch == 'Q') {
                produci((Messaggio){'M', ch, 0, 0, 0});     // M se riceve un comando di movimento
            } 
            else if (ch == ' ') {                           // L se riceve una richiesta di lancio
                produci((Messaggio){'L', 0, 0, 0, 0});
            }
        }
        usleep(16000);
    }
    return NULL;
}


// Gestore dedicato ai proiettili ('P' è lo sparo effettivo, 'K' quando il proiettile deve essere distrutto, 'D' se esce dallo schermo, 'U' quando la lista va aggiornata, 'Z' se vanno eliminati tutti)
void* thread_gestore_proiettili(void *args) {
    GranateProiettili *m_args = (GranateProiettili*)args; 
    int max_x_val = m_args->max_x_val; 
    free(args);
    NodoProiettile *lista_interna = NULL;
    long prossimo_id = 0;

    while (partita_in_corso) {
        Messaggio cmd;
        while (preleva_comando(&coda_proiettili, &cmd)) {
            if (cmd.tipo == 'P') {
                NodoProiettile *nuovo = malloc(sizeof(NodoProiettile));
                if (nuovo) {
                    nuovo->id = prossimo_id; 
                    prossimo_id += 2; 
                    nuovo->tipo = 'P';
                    nuovo->x = cmd.x; 
                    nuovo->y = cmd.y; 
                    nuovo->dx = cmd.dx;
                    nuovo->da_rimuovere = false; 
                    nuovo->next = lista_interna;
                    lista_interna = nuovo;
                }
            } 
            else if (cmd.tipo == 'K') {
                for (NodoProiettile *p = lista_interna; p; p = p->next) {
                    if (p->id == cmd.id) { 
                        p->da_rimuovere = true; 
                        break; 
                    }
                }
            } 
            else if (cmd.tipo == 'Z') {
                for (NodoProiettile *p = lista_interna; p; p = p->next) {
                    p->da_rimuovere = true;
                }
            }
        }

        for (NodoProiettile *p = lista_interna; p; p = p->next) {
            if (!p->da_rimuovere) {
                p->x += p->dx;
                if (p->x < -1 || p->x > max_x_val) {
                    p->da_rimuovere = true;
                    produci((Messaggio){'D', p->id, 0, 0, 'P'});
                } 
                else {
                    produci((Messaggio){'U', p->id, p->x, p->y, 'P'});
                }
            }
        }

        NodoProiettile **puntatore = &lista_interna;
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
    while(lista_interna) { 
        NodoProiettile* temp = lista_interna; 
        lista_interna = lista_interna->next; 
        free(temp); 
    }
    return NULL;
}


// Gestore dedicato alle granate ('G' è il lancio effettivo, 'K' quando la granata deve essere distrutta, 'D' se esce dallo schermo, 'U' quando la lista va aggiornata, 'Z' se vanno eliminati tutti)
void* thread_gestore_granate(void *args) {
    GranateProiettili *m_args = (GranateProiettili*)args; 
    int max_x_val = m_args->max_x_val; 
    free(args);
    NodoProiettile *lista_interna = NULL;
    long prossimo_id = 1;

    while (partita_in_corso) {
        Messaggio cmd;
        while (preleva_comando(&coda_granate, &cmd)) {
            if (cmd.tipo == 'G') {
                //Granata sinistra
                NodoProiettile *nuovo_sx = malloc(sizeof(NodoProiettile));
                if (nuovo_sx) {
                    nuovo_sx->id = prossimo_id; 
                    prossimo_id += 2; 
                    nuovo_sx->tipo = 'G';
                    nuovo_sx->x = cmd.x; 
                    nuovo_sx->y = cmd.y + 1;
                    nuovo_sx->dx = -1;
                    nuovo_sx->da_rimuovere = false; 
                    nuovo_sx->next = lista_interna;
                    lista_interna = nuovo_sx;
                }
                // Granata destra
                NodoProiettile *nuovo_dx = malloc(sizeof(NodoProiettile));
                if (nuovo_dx) {
                    nuovo_dx->id = prossimo_id; 
                    prossimo_id += 2; 
                    nuovo_dx->tipo = 'G';
                    nuovo_dx->x = cmd.x + 1; 
                    nuovo_dx->y = cmd.y + 1; 
                    nuovo_dx->dx = 1;
                    nuovo_dx->da_rimuovere = false; 
                    nuovo_dx->next = lista_interna;
                    lista_interna = nuovo_dx;
                }
            } else if (cmd.tipo == 'K') {
                for (NodoProiettile *p = lista_interna; p; p = p->next) {
                    if (p->id == cmd.id) { 
                        p->da_rimuovere = true; 
                        break; 
                    }
                }
            } else if (cmd.tipo == 'Z') {
                for (NodoProiettile *p = lista_interna; p; p = p->next) {
                    p->da_rimuovere = true;
                }
            }
        }

        for (NodoProiettile *p = lista_interna; p; p = p->next) {
            if (!p->da_rimuovere) {
                p->x += p->dx * 2;
                if (p->x < -1 || p->x > max_x_val) {
                    p->da_rimuovere = true;
                    produci((Messaggio){'D', p->id, 0, 0, 'G'});
                } 
                else {
                    produci((Messaggio){'U', p->id, p->x, p->y, 'G'});
                }
            }
        }
        
        NodoProiettile **puntatore = &lista_interna;
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
    while(lista_interna) { 
        NodoProiettile* temp = lista_interna; 
        lista_interna = lista_interna->next; 
        free(temp); 
    }
    return NULL;
}