#ifndef COMMAND_H
#define COMMAND_H

#include <arpa/inet.h>
#include <pthread.h>

// enum per i tipi di comando
typedef enum {
    ERR,

    // client -> server
    CREATE_CARD,
    HELLO,
    QUIT,
    PONG_LAVAGNA,
    ACK_CARD,
    REQUEST_USER_LIST,
    CARD_DONE,

    // server -> client
    SEND_USER_LIST,
    PING_USER,
    HANDLE_CARD,

    // client -> client
    REVIEW_CARD,
    ACK_REVIEW
} CommandType;


// rappresenta lo stato di ricezione dei comandi per ogni client
typedef struct {
    char buf[1024]; // buffer di ricezione
    int start;      // inizio dati validi
    int end;        // fine dati validi
} RecvState;
// macro per il numero di tipi di comando
#define NUM_CMD_TYPES (ACK_REVIEW + 1)

// passa da stringa a tipo di comando
CommandType str_to_cmdtype(const char *keyword);

// passa da tipo di comando a stringa
const char *cmdtype_to_str(CommandType type);

// numero massimo di argomenti per comando
#define MAX_CMD_ARGS 20

// numero massimo di caratteri per comando
#define CMD_BUF_SIZE 256

// rappresenta un comando, con tipo e lista di argomenti
typedef struct {
    CommandType type;
    const char *args[MAX_CMD_ARGS];
} Command;

// ottiene il numero di argomenti di un comando
int get_num_args(const Command *cm);

// mette un comando come stringa su un buffer
void command_to_buf(const Command *cm, char *buf);

// prende un comando come stringa da un buffer
void buf_to_command(char *buf, Command *cm);

// invia un comando su un socket TCP, bloccando un mutex se fornito
int send_command(const Command *cm, int sock, pthread_mutex_t *m);

// riceve un comando da un socket TCP, bloccando un mutex se fornito
int recv_command(Command *cm, int sock, pthread_mutex_t *m, RecvState* state);

// invia un comando su un socket UDP
int sendto_command(const Command *cm, int sock, const struct sockaddr_in *addr);

// riceve un comando da un socket UDP
int recvfrom_command(Command *cm, int sock, unsigned short *port);

#endif
