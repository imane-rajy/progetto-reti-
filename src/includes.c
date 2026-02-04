#include "includes.h"
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    CommandType type;
    const char *str;
} CommandEntry;

CommandEntry cmd_table[] = {
    // client -> server
    {CREATE_CARD, "CREATE_CARD"},
    {HELLO, "HELLO"},
    {QUIT, "QUIT"},
    {PONG_LAVAGNA, "PONG_LAVAGNA"},
    {ACK_CARD, "ACK_CARD"},
    {REQUEST_USER_LIST, "REQUEST_USER_LIST"},
    {CARD_DONE, "CARD_DONE"},

    // server -> done
    {SEND_USER_LIST, "SEND_USER_LIST"},
    {PING_USER, "PING_USER"},
    {HANDLE_CARD, "HANDLE_CARD"},
    {OK, "OK"},
    {ERR, "ERR"},

    // console -> client
    {SHOW_LAVAGNA, "SHOW_LAVAGNA"},
    {SHOW_CLIENTS, "SHOW_CLIENTS"},
    {MOVE_CARD, "MOVE_CARD"},

    // client -> client
    {REVIEW_CARD, "REVIEW_CARD"},
    {ACK_REVIEW_CARD, "ACK_REVIEW_CARD"}};

CommandType str_to_type(const char *str) {
    if (str == NULL)
        return ERR;

    for (int i = 0; i < NUM_CMD_TYPES; i++) {
        const CommandEntry *entry = &cmd_table[i];
        if (strcmp(entry->str, str) == 0) {
            return entry->type;
        }
    }

    return ERR;
}

const char *type_to_str(CommandType type) { return cmd_table[type].str; }

int get_argc(const Command *cm) {
    int i = 0;
    while (i < MAX_CMD_ARGS) {
        if (cm->args[i] == NULL) {
            break;
        }
        i++;
    }

    return i;
}

void cmd_to_buf(const Command *cm, char *buf) {
    int pos = 0;

    // copia tipo
    const char *type_str = type_to_str(cm->type);
    pos += snprintf(buf, CMD_BUF_SIZE, "%s", type_str);

    // copia gli argomenti
    for (int i = 0; i < get_argc(cm) && pos < CMD_BUF_SIZE; i++) {
        pos += snprintf(buf + pos, CMD_BUF_SIZE - pos, " %s", cm->args[i]);
    }
}

void buf_to_cmd(char *buf, Command *cm) {
    // tokenizza il tipo
    char *token = strtok(buf, " ");
    cm->type = str_to_type(token);

    // tokenizza gli argomenti
    int argc = 0;
    while ((token = strtok(NULL, " ")) && argc < MAX_CMD_ARGS) {
        cm->args[argc++] = token;
    }
}

int send_command(const Command *cm, int sock, pthread_mutex_t *m) {
	if(m != NULL) pthread_mutex_lock(m);
	
	// metti comando su buffer
    char buf[CMD_BUF_SIZE];
    cmd_to_buf(cm, buf);

	// invia buffer
    int ret = send(sock, &buf, strlen(buf), 0);
    
	if(m != NULL) pthread_mutex_unlock(m);
    
	return ret;
}

int recv_command(Command *cm, char *buf, int sock, pthread_mutex_t *m) {
	if(m != NULL) pthread_mutex_lock(m);
	
	// prendi buffer
    int ret = recv(sock, buf, sizeof(buf), 0);

	// scrivi comando da buffer
    buf_to_cmd(buf, cm);
	
	if(m != NULL) pthread_mutex_unlock(m);
    return ret;
}
