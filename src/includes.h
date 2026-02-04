#ifndef INCLUDES_H
#define INCLUDES_H

#include <pthread.h>

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
int recv_command(Command *cm, char *buf, int sock, pthread_mutex_t *m);

#endif
