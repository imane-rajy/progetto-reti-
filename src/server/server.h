#ifndef SERVER_H
#define SERVER_H

#include "../command.h"

int send_client(const Command *cm, unsigned short port);

#endif
