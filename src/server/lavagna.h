#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../utils/card.h"
#include "../utils/command.h"
#include <time.h>

// inizializza la lavagna con le prime 10 card
void init_lavagna();

// gestisce un comando inviato da un certo client
void gestisci_comando(const Command *cmd, unsigned short port);

// chiamata ogni secondo per gestire i PING
int gestisci_ping(pthread_mutex_t *server_user_m);

#endif
