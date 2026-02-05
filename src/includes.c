#include "includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void card_to_cmd(const Card *c, Command *cm) {
	static char id[16], col[8], user[16];
  	static char date[32], time[16];
  	static char testo[MAX_TESTO];

  	snprintf(id, sizeof(id), "%d", c->id);
  	snprintf(col, sizeof(col), "%d", c->colonna);
  	snprintf(user, sizeof(user), "%d", c->utente);

	snprintf(date, sizeof(date), "%02d-%02d-%04d",
		c->timestamp.tm_mday,
		c->timestamp.tm_mon + 1,
		c->timestamp.tm_year + 1900);

  	snprintf(time, sizeof(time), "%02d:%02d:%02d",
		c->timestamp.tm_hour,
		c->timestamp.tm_min,
		c->timestamp.tm_sec);

  	strncpy(testo, c->testo, MAX_TESTO - 1);
  	testo[MAX_TESTO - 1] = '\0';

  	cm->args[0] = id;
  	cm->args[1] = col;
  	cm->args[2] = user;
  	cm->args[3] = date;
  	cm->args[4] = time;  	
	cm->args[5] = testo;
}

int cmd_to_card(const Command *cm, Card *c) {
	if (get_argc(cm) < 6) return -1;

	int day, mon, year, hour, min, sec;

	c->id = atoi(cm->args[0]);
	c->colonna = (Colonna) atoi(cm->args[1]);
	c->utente = atoi(cm->args[2]);

	sscanf(cm->args[3], "%d-%d-%d", &day, &mon, &year);
	sscanf(cm->args[4], "%d:%d:%d", &hour, &min, &sec);

	c->timestamp.tm_mday = day;
	c->timestamp.tm_mon = mon - 1;
	c->timestamp.tm_year = year - 1900;
	c->timestamp.tm_hour = hour;
	c->timestamp.tm_min = min;
	c->timestamp.tm_sec = sec;

	char* pun = c->testo;
	for(int i = 5; i < get_argc(cm); i++) {
		const char* arg = cm->args[i];
		
		strcpy(pun, arg);
		pun += strlen(arg);
		
		*pun++ = ' ';
	}

	return 0;
}

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
    if (m != NULL) pthread_mutex_lock(m);

    // metti comando su buffer
    char buf[CMD_BUF_SIZE + 1] = {0};
    cmd_to_buf(cm, buf);
	buf[strlen(buf)] = '\n'; // delimitatore: a capo

    // invia buffer
    int ret = send(sock, &buf, strlen(buf), 0);

    if (m != NULL) pthread_mutex_unlock(m);

    return ret;
}

int recv_command(Command *cm, int sock, pthread_mutex_t *m) {
    static char recvbuf[1024]; 
	static int recvidx = 0;

	if (m != NULL) pthread_mutex_lock(m);

	int ret;
	while(1) {
		// prendi buffer
		ret = recv(sock, recvbuf, sizeof(recvbuf), 0);
		
		// gestisci errori di lettura
		if (ret <= 0) break;
		
		// aggiungi i byte letti
		recvidx += ret;
		
		int done = 0;
		for (int i = 0; i < recvidx; i++) {
			// se trovi \n è una linea
			if (recvbuf[i] == '\n') {
				recvbuf[i] = '\0';
	
				// scrivi comando da buffer
				buf_to_cmd(recvbuf, cm);
				done = 1;
				break;
			}
		}	

		if(done) break;
	}

    if (m != NULL) pthread_mutex_unlock(m);

	return ret;
}

int get_user_cards(unsigned short client, Card user_cards[MAX_CARDS]) {

    int n = 0;

    for (int i = 0; i < num_card && n < MAX_CARDS; i++) {
        if (cards[i].client == client) {
            user_cards[n++] = cards[i]; // copio solo le card dell’utente
        }
    }

    return n; // ritorna quante card ci sono nel “sotto-array”
}
