#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../includes.h"
#include <time.h>

#define MAX_CARDS 100
#define MAX_TESTO 256

typedef enum {
	TO_DO,
	DOING,
	DONE
} Colonna;

#define NUM_COLS (DONE + 1)

Colonna str_to_col(const char *str);
const char *col_to_str(Colonna id);

typedef struct {
    int id;
    Colonna colonna; 
    char testo[MAX_TESTO];
    int utente; 
    struct tm timestamp;
} Card;

extern Card cards[MAX_CARDS];
extern int num_card;

void init_lavagna();
void mostra_lavagna();
void gestisci_comando(const Command*, unsigned short);

#endif
