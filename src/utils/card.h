#ifndef CARD_H
#define CARD_H

#include "command.h"

// numero massimo di caratteri nel testo di una card
#define MAX_TESTO 256

// enum per le colonne delle card
typedef enum { TO_DO, DOING, DONE } Colonna;

// macro per il numero di colonne
#define NUM_COLS (DONE + 1)

// passa da stringa a colonna
Colonna str_to_col(const char *str);

// passa da colonna a stringa
const char *col_to_str(Colonna id);

// rappresenta una card
typedef struct {
    int id;
    Colonna colonna;
    char testo[MAX_TESTO];
    unsigned short port;
    struct tm timestamp;
} Card;

// mette una card su un comando
void card_to_cmd(const Card *c, Command *cm);

// prende una card da un comando
int cmd_to_card(const Command *cm, Card *c);

// imposta il timestamp di una card alla data/ora corrente
void timestamp_card(Card *card);

#endif
