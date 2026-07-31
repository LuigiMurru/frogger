#include "frogger.h"

// Utilizziamo come base l'algoritmo AABB (Axis-Aligned Bounding Box): collisione fra due rettangoli
bool collisione_rettangoli(int r1_x, int r1_y, int r1_larghezza, int r1_altezza, int r2_x, int r2_y, int r2_larghezza, int r2_altezza) {
    return (r1_x < r2_x + r2_larghezza &&   // Il lato sinistro di R1 è a sinistra del lato destro di R2
            r1_x + r1_larghezza > r2_x &&   // Il lato destro di R1 è a destra del lato sinistro di R2
            r1_y < r2_y + r2_altezza &&     // Il lato superiore di R1 è sopra il lato inferiore di R2
            r1_y + r1_altezza > r2_y);      // Il lato inferiore di R1 è sotto il lato superiore di R2
}


// Previene il "respawn" del proiettile dopo che colpisce una granata (senza questa funzione il proiettile potrebbe riapparire)
void aggiungi_a_ignorati(long id_da_ignorare) {
    NodoIdIgnorati *nuovo = malloc(sizeof(NodoIdIgnorati));
    if (nuovo) {    
        nuovo->id = id_da_ignorare;
        nuovo->tempo_vita = 40;
        nuovo->next = lista_id_ignorati;
        lista_id_ignorati = nuovo;
    }
}


// Gestione collisioni elementi dinamici
void gestione_collisioni() {
    // Collisione proiettile vs rana
    for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
        if (p->tipo == 'P' && !p->da_rimuovere) {
            if (collisione_rettangoli(p->x, p->y, 1, 1, rana_x, rana_y, LARGHEZZA_RANA, ALTEZZA_RANA)) {
                punteggio = (punteggio >= 10) ? punteggio - 10 : 0;
                vite--;
                reset_rana(pipe_a_cocco[1], pipe_produttori_a_consumatore[1], pipe_a_gestore_proiettili[1], pipe_a_gestore_granate[1]);
                p->da_rimuovere = true;

                Messaggio msg_kill = {'K', p->id, 0, 0, 0};
                // Invia il comando al gestore dei proiettili
                write(pipe_a_gestore_proiettili[1], &msg_kill, sizeof(Messaggio));
                
                aggiungi_a_ignorati(p->id);
            }
        }
    }
    
    // Collisione granata vs proiettile
    for (NodoProiettile *g = lista_proiettili; g; g = g->next) {
        if (g->tipo == 'G' && !g->da_rimuovere) {
            for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
                if (p->tipo == 'P' && !p->da_rimuovere) {
                    if (collisione_rettangoli(g->x, g->y, 1, 1, p->x, p->y, 1, 1)) {
                        g->da_rimuovere = true;
                        p->da_rimuovere = true;

                        // Invia il comando di kill per la granata al suo gestore
                        Messaggio msg_kill_g = {'K', g->id, 0, 0, 0};
                        write(pipe_a_gestore_granate[1], &msg_kill_g, sizeof(Messaggio));

                        // Invia il comando di kill per il proiettile al suo gestore
                        Messaggio msg_kill_p = {'K', p->id, 0, 0, 0};
                        write(pipe_a_gestore_proiettili[1], &msg_kill_p, sizeof(Messaggio));
                        
                        aggiungi_a_ignorati(g->id);
                        aggiungi_a_ignorati(p->id);
                    }
                }
            }
        }
    }
    rimuovi_p_g();
}