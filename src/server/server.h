#ifndef SERVER_H
#define SERVER_H

#include "../utils/command.h"

// invia un comando ad un client identificato dalla sua porta
int send_to_client(const Command *cmd, unsigned short port);

#endif
