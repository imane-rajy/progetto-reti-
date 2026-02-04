#ifndef LAVAGNA_H
#define LAVAGNA_H

#include <time.h>

#define MAX_CARDS 100
#define MAX_TESTO 256

typedef struct {
    int id;
    int colonna; 
    char testo[MAX_TESTO];
    int utente; 
    struct tm timestamp;
} Card;

extern Card cards[MAX_CARDS];
extern int num_card;

void init_lavagna();
void mostra_lavagna();
void gestisci_comando(char*, unsigned short);

#endif
