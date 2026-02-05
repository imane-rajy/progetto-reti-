#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../card.h"
#include "../command.h"
#include <time.h>

#define MAX_CARDS 100

#define MAX_USERS 30
#define MIN_PORT_USERS 5679
#define MIN_NUM_USER_DONE 2

#define WAIT_TO_PING 5
#define WAIT_FOR_PONG 10

typedef enum {
    BUSY,
    IDLE,
    ASSIGNED_CARD
} UserState;

typedef struct {
    unsigned short port;
    UserState state;
    int card_id;
    int timer_ping;
    int ping_sent;
} User;

extern Card cards[MAX_CARDS];
extern int num_cards;

void init_lavagna();
void mostra_lavagna();

void gestisci_comando(const Command *cmd, unsigned short port);
int gestici_ping(pthread_mutex_t *server_user_m);

#endif
