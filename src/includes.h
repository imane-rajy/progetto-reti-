#ifndef INCLUDES_H
#define INCLUDES_H

#include <pthread.h>

// comandi

typedef enum {
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
    OK,
    ERR,

    // console -> client
    SHOW_LAVAGNA,
    SHOW_CLIENTS,
    MOVE_CARD,

    // client -> client
    REVIEW_CARD,
    ACK_REVIEW_CARD
} CommandType;

#define NUM_CMD_TYPES (ACK_REVIEW_CARD + 1)

CommandType str_to_type(const char *keyword);
const char *type_to_str(CommandType type);

#define MAX_CMD_ARGS 10
#define CMD_BUF_SIZE 256

typedef struct {
    CommandType type;
    const char *args[MAX_CMD_ARGS];
} Command;

int get_argc(const Command *cm);
void cmd_to_buf(const Command *cm, char *buf);
void buf_to_cmd(char *buf, Command *cm);

int send_command(const Command *cm, int sock, pthread_mutex_t *m);
int recv_command(Command *cm, int sock, pthread_mutex_t *m);

// card

#define MAX_TESTO 256
#define MAX_CARD_SIZE (MAX_TESTO + 256)

typedef enum { TO_DO, DOING, DONE } Colonna;

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

int get_user_cards(unsigned short client, Card user_cards[MAX_CARDS]);
void card_to_cmd(const Card *c, Command *cm);
int cmd_to_card(const Command *cm, Card *c);

#endif
