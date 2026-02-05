#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../card.h"
#include "../command.h"
#include <time.h>

#define MAX_CARDS 100

#define MAX_USERS 30

#define MIN_PORT_USERS 5679

typedef enum {

    BUSY,
    IDLE,
    ASSIGNED_CARD

} UserState;

typedef struct {
    unsigned short port;
    UserState state;
<<<<<<< HEAD
    Card* card;

=======
>>>>>>> f1dcf491bd6a8f096f5b58e6b416c8ff3206ad52
} User;

extern Card cards[MAX_CARDS];
extern int num_cards;

void init_lavagna();
void mostra_lavagna();

void gestisci_comando(const Command *cmd, unsigned short port);

#endif
