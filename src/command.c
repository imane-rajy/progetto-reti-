#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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
    if (str == NULL) return ERR;

    for (int i = 0; i < NUM_CMD_TYPES; i++) {
        // controlla tutte le entrate della command table
        const CommandEntry *entry = &cmd_table[i];

        if (strcmp(entry->str, str) == 0) { return entry->type; }
    }

    return ERR;
}

const char *type_to_str(CommandType type) {
    return cmd_table[type].str;
}

int get_argc(const Command *cm) {
    // conta gli argomenti in un comando
    int i = 0;
    while (i < MAX_CMD_ARGS) {
        if (cm->args[i] == NULL) { break; }

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
    if (m != NULL) pthread_mutex_lock(m); // blocca socket

    // metti comando su buffer
    char buf[CMD_BUF_SIZE + 1] = {0};
    cmd_to_buf(cm, buf);
    buf[strlen(buf)] = '\n'; // delimitatore: a capo

    // invia buffer
    int ret = send(sock, &buf, strlen(buf), 0);

    if (m != NULL) pthread_mutex_unlock(m); // sblocca socket

    return ret;
}

int recv_command(Command *cm, int sock, pthread_mutex_t *m) {
    // inizializza buffer di ricezione
    static char recvbuf[1024];
    static int start = 0; // inizio di dati validi nel buffer
    static int end = 0;   // fine di dati validi nel buffer

    if (m) pthread_mutex_lock(m); // blocca socket

    int ret = 0;

    while (1) {
        // cerca un delimitatore nel range valido
        for (int i = start; i < end; i++) {
            if (recvbuf[i] == '\n') {
                // trovata una linea, crea un comando
                recvbuf[i] = '\0';
                buf_to_cmd(recvbuf + start, cm);

                // porta start dopo la fine della linea trovata
                start = i + 1;

                // se è vuoto, riparti dall'origine
                if (start == end) { start = end = 0; }

                // sblocca e restituisci il comando
                if (m) pthread_mutex_unlock(m);
                return 1;
            }
        }

        // gestisci l'overlow svuotando il buffer
        if (end == sizeof(recvbuf)) { start = end = 0; }

        // leggi altri dati se non c'è ancora un delimitatore
        ret = recv(sock, recvbuf + end, sizeof(recvbuf) - end, 0);
        if (ret <= 0) break;

        end += ret;
    }

    if (m) pthread_mutex_unlock(m); // sblocca socket
    return ret;
}
