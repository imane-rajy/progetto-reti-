#include "card.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *col_names[] = {"TO_DO", "DOING", "DONE"};

// passa da stringa a colonna
Colonna str_to_col(const char *str) {
    for (int i = 0; i < NUM_COLS; i++) {
        // controlla tutte le colonne
        if (strcmp(col_names[i], str) == 0) {
            return i;
        }
    }

    return 0;
}

// passa da colonna a stringa
const char *col_to_str(Colonna id) {
    return col_names[id];
}

// mette una card su un comando
void card_to_cmd(const Card *c, Command *cm) {
    // inizializza buffer statici
    static char id[16], col[8], user[16];
    static char date[32], time[16];
    static char testo[MAX_TESTO];

    // stampa dati sui buffer
    snprintf(id, sizeof(id), "%d", c->id);
    snprintf(col, sizeof(col), "%d", c->colonna);
    snprintf(user, sizeof(user), "%d", c->port);

    snprintf(date, sizeof(date), "%02d-%02d-%04d", c->timestamp.tm_mday, c->timestamp.tm_mon + 1,
             c->timestamp.tm_year + 1900);

    snprintf(time, sizeof(time), "%02d:%02d:%02d", c->timestamp.tm_hour, c->timestamp.tm_min, c->timestamp.tm_sec);

    // copia il testo nell'ultimo buffer
    strncpy(testo, c->testo, MAX_TESTO - 1);
    testo[MAX_TESTO - 1] = '\0';

    // imposta gli argomenti coi buffer
    cm->args[0] = id;
    cm->args[1] = col;
    cm->args[2] = user;
    cm->args[3] = date;
    cm->args[4] = time;
    cm->args[5] = testo;
}

// prende una card da un comando
int cmd_to_card(const Command *cm, Card *c) {
    // controlla che ci siano abbastanza argomenti
    if (get_argc(cm) < 6)
        return -1;

    // prendi campi banali
    c->id = atoi(cm->args[0]);
    c->colonna = (Colonna)atoi(cm->args[1]);
    c->port = atoi(cm->args[2]);

    // inizializza campi della data
    int day, mon, year, hour, min, sec;

    // leggi nei campi della data
    sscanf(cm->args[3], "%d-%d-%d", &day, &mon, &year);
    sscanf(cm->args[4], "%d:%d:%d", &hour, &min, &sec);

    // imposta la data
    c->timestamp.tm_mday = day;
    c->timestamp.tm_mon = mon - 1;
    c->timestamp.tm_year = year - 1900;
    c->timestamp.tm_hour = hour;
    c->timestamp.tm_min = min;
    c->timestamp.tm_sec = sec;

    // copia tutti gli ultimi argomenti nel testo
    char *pun = c->testo;
    int argc = get_argc(cm);

    for (int i = 5; i < argc; i++) {
        const char *arg = cm->args[i];
        size_t len = strlen(arg);

        // nel caso di overflow esci
        if ((pun - c->testo) + len >= MAX_TESTO - 1)
            break;

        strcpy(pun, arg);
        pun += len;

        if (i != argc - 1 && (pun - c->testo) < MAX_TESTO - 1) {
            *pun++ = ' ';
        }
    }

    // termina
    *pun = '\0';

    return 0;
}

// imposta il timestamp di una card alla data/ora corrente
void timestamp_card(Card *card) {
    time_t now = time(NULL);
    card->timestamp = *localtime(&now);
}
