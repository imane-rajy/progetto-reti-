#ifndef LAVAGNA_H
#define LAVAGNA_H

#include "../includes.h"
#include <time.h>

#define MAX_CARDS 100

extern Card cards[MAX_CARDS];
extern int num_card;

void init_lavagna();
void mostra_lavagna();
void gestisci_comando(const Command *cmd, unsigned short port);

#endif
