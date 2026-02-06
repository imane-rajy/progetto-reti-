#include "command.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

// entrata della tabella dei comandi
typedef struct {
    CommandType type;
    const char *str;
} CommandEntry;

// tabella dei comandi, associa ogni tipo alla stringa che lo rappresenta
CommandEntry cmd_table[] = {{ERR, "ERR"},

                            // client -> server
                            {CREATE_CARD, "CREATE_CARD"},
                            {HELLO, "HELLO"},
                            {QUIT, "QUIT"},
                            {PONG_LAVAGNA, "PONG_LAVAGNA"},
                            {ACK_CARD, "ACK_CARD"},
                            {REQUEST_USER_LIST, "REQUEST_USER_LIST"},
                            {CARD_DONE, "CARD_DONE"},

                            // server -> client
                            {SEND_USER_LIST, "SEND_USER_LIST"},
                            {PING_USER, "PING_USER"},
                            {HANDLE_CARD, "HANDLE_CARD"},

                            // client -> client
                            {REVIEW_CARD, "REVIEW_CARD"},
                            {ACK_REVIEW_CARD, "ACK_REVIEW_CARD"}};

// passa da stringa a tipo di comando
CommandType str_to_cmdtype(const char *str) {
    if (str == NULL) {
        return ERR;
    }

    for (int i = 0; i < NUM_CMD_TYPES; i++) {
        // controlla tutte le entrate della command table
        const CommandEntry *entry = &cmd_table[i];

        if (strcmp(entry->str, str) == 0) {
            return entry->type;
        }
    }

    return ERR;
}

// passa da tipo di comando a stringa
const char *cmdtype_to_str(CommandType type) {
    return cmd_table[type].str;
}

// ottiene il numero di argomenti di un comando
int get_argc(const Command *cmd) {
    // conta gli argomenti in un comando
    int i = 0;
    while (i < MAX_CMD_ARGS) {
        if (cmd->args[i] == NULL) {
            break;
        }

        i++;
    }

    return i;
}

// mette un comando come stringa su un buffer
void cmd_to_buf(const Command *cmd, char *buf) {
    int pos = 0;

    // copia tipo
    const char *type_str = cmdtype_to_str(cmd->type);
    pos += snprintf(buf, CMD_BUF_SIZE, "%s", type_str);

    // copia gli argomenti
    for (int i = 0; i < get_argc(cmd) && pos < CMD_BUF_SIZE; i++) {
        pos += snprintf(buf + pos, CMD_BUF_SIZE - pos, " %s", cmd->args[i]);
    }
}

// prende un comando come stringa da un buffer
void buf_to_cmd(char *buf, Command *cmd) {
    // tokenizza il tipo
    char *token = strtok(buf, " ");
    cmd->type = str_to_cmdtype(token);

    // tokenizza gli argomenti
    int argc = 0;
    while ((token = strtok(NULL, " ")) && argc < MAX_CMD_ARGS) {
        cmd->args[argc++] = token; // l'argomento è un riferimento al buffer fornito!
    }
}

// invia un comando su un socket TCP, bloccando un mutex se fornito
int send_command(const Command *cm, int sock, pthread_mutex_t *m) {
    if (m != NULL) {
        pthread_mutex_lock(m); // blocca socket
    }

    // metti comando su buffer
    char buf[CMD_BUF_SIZE + 1] = {0};
    cmd_to_buf(cm, buf);
    buf[strlen(buf)] = '\n'; // delimitatore: a capo

    // invia buffer
    int ret = send(sock, &buf, strlen(buf), 0);

    if (m != NULL) {
        pthread_mutex_unlock(m); // sblocca socket
    }

    return ret;
}

// riceve un comando da un socket TCP, bloccando un mutex se fornito
int recv_command(Command *cm, int sock, pthread_mutex_t *m) {
    // inizializza buffer di ricezione
    static char recvbuf[1024];
    static int start = 0; // inizio di dati validi nel buffer
    static int end = 0;   // fine di dati validi nel buffer

    if (m) {
        pthread_mutex_lock(m); // blocca socket
    }

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
                if (start == end) {
                    start = end = 0;
                }

                // sblocca e restituisci il comando
                if (m) {
                    pthread_mutex_unlock(m); // sblocca socket
                }
                return 1;
            }
        }

        // gestisci l'overlow svuotando il buffer
        if (end == sizeof(recvbuf)) {
            start = end = 0;
        }

        // leggi altri dati se non c'è ancora un delimitatore
        ret = recv(sock, recvbuf + end, sizeof(recvbuf) - end, 0);
        if (ret <= 0) {
            break;
        }

        end += ret;
    }

    if (m) {
        pthread_mutex_unlock(m); // sblocca socket
    }
    return ret;
}

// invia un comando su un socket UDP
int sendto_command(const Command *cm, int sock, const struct sockaddr_in *addr) {
    // metti comando su buffer
    char buf[CMD_BUF_SIZE];
    cmd_to_buf(cm, buf);

    // invia buffer
    return sendto(sock, buf, strlen(buf), 0, (const struct sockaddr *)addr, sizeof(*addr));
}

// riceve un comando da un socket UDP
int recvfrom_command(Command *cm, int sock, unsigned short *port) {
    // imposta indirizzo
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buf[CMD_BUF_SIZE + 1];

    // ricevi un comando
    int ret = recvfrom(sock, buf, CMD_BUF_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
    if (ret < 0) {
        return ret;
    }

    buf[ret] = '\0';

    // ottieni porta
    *port = ntohs(addr.sin_port);

    // crea un comando
    buf_to_cmd(buf, cm);
    return ret;
}
