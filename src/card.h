#ifndef CARD_H
#define CARD_H

#include "command.h"

#define MAX_TESTO 256
#define MAX_CARD_SIZE (MAX_TESTO + 256)

typedef enum { TO_DO, DOING, DONE } Colonna;
extern const char *col_names[];

#define NUM_COLS (DONE + 1)

Colonna str_to_col(const char *str);
const char *col_to_str(Colonna id);

typedef struct {
    int id;
    Colonna colonna;
    char testo[MAX_TESTO];
    unsigned short client;
    struct tm timestamp;
} Card;

void card_to_cmd(const Card *c, Command *cm);
int cmd_to_card(const Command *cm, Card *c);

void timestamp_card(Card *card);

#endif
