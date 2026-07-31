#include "frogger.h"

// Chiamata quando si riceve un aggiornamento dal processo gestore
void aggiorna_proiettile(Messaggio msg) {
    for (NodoIdIgnorati *ign = lista_id_ignorati; ign; ign = ign->next) {
        if (ign->id == msg.id) {
            return;
        }
    }

    // Cerca se il proiettile con questo ID esiste già
    for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
        if (p->id == msg.id) {
            p->x = msg.x;
            p->y = msg.y;
            p->tipo = msg.tipo;
            p->da_rimuovere = false; // È stato aggiornato, quindi non va rimosso
            return;
        }
    }

    NodoProiettile *nuovo = malloc(sizeof(NodoProiettile));
    if(!nuovo) {
        return;
    }
    nuovo->id = msg.id;
    nuovo->tipo = msg.tipo;
    nuovo->x = msg.x;
    nuovo->y = msg.y;
    nuovo->dx = msg.dx;
    nuovo->da_rimuovere = false;
    nuovo->next = lista_proiettili;
    lista_proiettili = nuovo;
}

// Pulisce la lista locale del gioco
void rimuovi_p_g() {
    NodoProiettile **puntatore = &lista_proiettili;
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
}