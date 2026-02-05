#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../command.h"
#include "../card.h"
#include <time.h>

#define MAX_CARDS 100


#define MAX_USERS 30

typedef enum{

    BUSY,
    IDLE

} UserState;


typedef struct {
    unsigned short id;
    UserState state;

} User;


extern Card cards[MAX_CARDS];
extern int num_card;

void init_lavagna();
void mostra_lavagna();

int get_user_cards(unsigned short client, Card user_cards[MAX_CARDS]);
void gestisci_comando(const Command *cmd, unsigned short port);

#endif
