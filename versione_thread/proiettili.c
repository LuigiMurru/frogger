#include "frogger.h"

// Chiamata quando si riceve un aggiornamento dal gestore
void aggiorna_proiettile(Messaggio msg) {
    for (NodoProiettile *p = lista_proiettili; p; p = p->next) {
        if (p->id == msg.id) {
            p->x = msg.x;
            p->y = msg.y;
            return;
        }
    }

    NodoProiettile *nuovo = malloc(sizeof(NodoProiettile));
    if(!nuovo) {
        return;
    }
    nuovo->id = msg.id;
    nuovo->tipo = (char)msg.dx;
    nuovo->x = msg.x;
    nuovo->y = msg.y;
    nuovo->dx = 0;
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