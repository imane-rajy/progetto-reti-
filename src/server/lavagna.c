#include "lavagna.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

User users[MAX_USERS] = {0}; // array per i client registrati
int num_users = 0;

int inserisci_user(unsigned short client) {
    int idx = client - MIN_PORT_USERS;
    if (users[idx].port == 0) {
        users[idx].port = client;
        num_users++;
        return idx;
    }

    // errore: spazio esaurito
    return -1;
}

User *controlla_user(unsigned short client) {
    int idx = client - MIN_PORT_USERS;
    User *user = &users[idx];
    if (user->port == client) return user;

    // errore: user non presente
    return NULL;
}

int rimuovi_user(User *user) {
    if (user != NULL) {
        user->port = 0;
        num_users--;
        return 1;
    }

    // errore: user non presente
    return -1;
}

#define COL_WIDTH 60

Card cards[MAX_CARDS] = {0};
int num_cards = 0;

int request_user_list(User *user) {
    // prepara buffer per id client
    char client_ports[num_users][6]; // 5 caratteri + terminatore per 65535

    // prepara risposta
    Command cm = {
        .type = SEND_USER_LIST
        // verrà popolato in seguito
    };

    // itera sui client
    int n = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (user == &users[i]) { continue; }

        if (users[i].port != 0) {
            // copia id client nel buffer
            snprintf(client_ports[n], 6, "%d", users[i].port);

            // usa come argomento
            cm.args[n] = client_ports[n];
            n++;
        }

        if (users[i].port != 0) {
            // copia id client nel buffer
            snprintf(client_ports[n], 6, "%d", users[i].port);

            // usa come argomento
            cm.args[n] = client_ports[n];
            n++;
        }
    }

    // invia risposta
    send_client(&cm, user->port);

    return 0;
}

void handle_card(User *user) {
    for (int i = 0; i < MAX_CARDS; i++) {
        // solo se esiste
        if (cards[i].id == 0) continue;

        // solo se è in TO_DO
        if (cards[i].colonna != TO_DO) continue;

        // prendi la card
        Card *card = &cards[i];

        // assegnala al client
        card->client = user->port;
        user->state = ASSIGNED_CARD;
        user->card_id = card->id;
        timestamp_card(card);

        // invia il comando HANDLE_CARD
        Command cmd = {.type = HANDLE_CARD};
        card_to_cmd(card, &cmd);
        send_client(&cmd, user->port);

        // invia lista utenti
        request_user_list(user);

        printf("Assegnata card %d a client %d\n", card->id, user->port);
        return;
    }
}

void handle_cards() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].state == IDLE) handle_card(&users[i]);
    }
}

int create_card(int id, Colonna colonna, const char *testo) {
    // verifica che l'id sia unico
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].id == id) { return -1; }
    }

    // alloca la card
    int idx = num_cards;
    num_cards++;
    if (idx >= MAX_CARDS) { return -2; }

    // imposta i dati della card
    cards[idx].id = id;
    cards[idx].colonna = colonna;

    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';

    printf("Creata card %d\n", id);

    handle_cards(); // fai il push della card
    return idx;
}

int hello(unsigned short client) {
    // prova a registrate un utente
    int idx = inserisci_user(client);

    if (idx >= 0) {
        printf("Registrato client %d\n", client);
        handle_card(&users[idx]);
        return 0;
    }

    return -1;
}

int find_card(int card_id) {
    // scansiona le card
    for (int i = 0; i < MAX_CARDS; i++) {
        // restituisci l'indice dato l'id
        if (cards[i].id == card_id) return i;
    }

    return -1;
}

int move_card(int card_id, Colonna to) {
    // trova l'indice della card
    int idx = find_card(card_id);
    if (idx < 0) { return -1; }

    // assicurati che non sia già li
    if (cards[idx].colonna == to) { return -2; }

    // sposta
    cards[idx].colonna = to;
    return 0;
}

int quit(User *user) {
    // ricorda la porta
    int port = user->port;

    // rimuovi l'utente
    int ret = rimuovi_user(user);

    // se ha avuto successo, rimuovi la sua card
    if (ret >= 0) {
        // trova la sua card
        int idx = find_card(user->card_id);
        if (idx < 0) { return -1; }

        // mettila in TO_DO
        Card *card = &cards[idx];
        if (card->colonna == DOING) {
            move_card(card->id, TO_DO);
            handle_cards(); // fai il push della card
        }

        printf("Deregistrato client %d\n", port);
        return 0;
    }

    return -1;
}

int ack_card(User *user, int card_id) {
    // controlla che sia stata assegnata una card
    if (user->state != ASSIGNED_CARD) return -1;

    // ottieni la card
    int idx = find_card(user->card_id);

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) { return -1; }

    // controlla che la card siain TO_DO
    if (cards[idx].colonna != TO_DO) { return -1; }

    // aggiorna lo stato della card e dell'utente
    cards[idx].colonna = DOING;
    user->state = BUSY;

    printf("Ricevuto ACK per card %d\n", card_id);
    return 0;
}

int card_done(User *user, int card_id) {
    // controlla che stia gestendo una card
    if (user->state != BUSY) { return -1; }

    // ottieni la card
    int idx = find_card(user->card_id);

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) { return -1; }

    // controlla che la card sia in DOING
    if (cards[idx].colonna != DOING) { return -1; }

    // aggiorna lo stato della card e dell'utente
    cards[idx].colonna = DONE;
    user->state = IDLE;
    timestamp_card(&cards[idx]);

    // invia un altra card
    handle_card(user);

    printf("Ricevuto CARD_DONE per card %d\n", card_id);
    return 0;
}




int pong_lavagna(){

}


// GESTISCE IL COMANDO IN ARRIVO DAL CLIENT!!!!!!
void gestisci_comando(const Command *cmd, unsigned short port) {
    // mostra_lavagna();

    // controlla che si registrato o che si stia registrando adesso
    User *user = controlla_user(port);

    if (user == NULL && cmd->type != HELLO) {
        printf("Ottenuto comando non HELLO (%d) da client non registrato %d\n", cmd->type, port);
        return;
    }

    int ret = -1;

    switch (cmd->type) {
    case CREATE_CARD: {
        int id = atoi(cmd->args[0]);
        Colonna colonna = str_to_col(cmd->args[1]);
        const char *testo = cmd->args[2]; // prendi tutti gli ultimi argomenti

        ret = create_card(id, colonna, testo);
        break;
    }
    case HELLO: {
        ret = hello(port);
        break;
    }
    case QUIT: {
        ret = quit(user);
        break;
    }
    case PONG_LAVAGNA: {
        // pong_lavagna();
        break;
    }
    case ACK_CARD: {
        if (get_argc(cmd) < 1) break;

        int card_id = atoi(cmd->args[0]);
        ret = ack_card(user, card_id);
        break;
    }
    case REQUEST_USER_LIST: {
        ret = request_user_list(user);
        break;
    }
    case CARD_DONE: {
        if (get_argc(cmd) < 1) break;

        int card_id = atoi(cmd->args[0]);
        ret = card_done(user, card_id);
        break;
    }
    default:
        break;
    }

    if (ret < 0) { printf("Errore nell'esecuzione del comando %d del client %d\n", cmd->type, port); }
}

void mostra_lavagna() {
    // system("clear");

    // stampa header
    for (int c = 0; c < NUM_COLS; c++) {
        printf("%-*s", COL_WIDTH, col_names[c]);
    }
    printf("\n");
    for (int i = 0; i < NUM_COLS * COL_WIDTH; i++) {
        printf("-");
    }
    printf("\n");

    // ottieni il numero di righe
    int max_rows = 0;
    int col_counts[NUM_COLS] = {0};
    for (int i = 0; i < num_cards; i++) {
        col_counts[cards[i].colonna]++;
        if (col_counts[cards[i].colonna] > max_rows) max_rows = col_counts[cards[i].colonna];
    }

    // realizza una tabella di puntatori a card organizzata per colonne
    Card *col_cards[NUM_COLS][MAX_CARDS] = {0};
    int col_index[NUM_COLS] = {0};
    for (int i = 0; i < num_cards; i++) {
        Colonna col = cards[i].colonna;
        col_cards[col][col_index[col]++] = &cards[i];
    }

    // stampa le card
    for (int row = 0; row < max_rows; row++) {
        // 2 righe per card
        for (int l = 0; l < 3; l++) {
            for (int c = 0; c < NUM_COLS; c++) {
                if (row < col_index[c]) {
                    Card *card = col_cards[c][row];
                    char buf[COL_WIDTH + 1];

                    switch (l) {
                    // colonna 0: testo
                    case 0:
                        strncpy(buf, card->testo, COL_WIDTH);
                        printf("%-*s", COL_WIDTH, buf);
                        break;

                    // colonna 1: altri dati
                    case 1:
                        snprintf(buf, sizeof(buf), "ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d", card->id, card->client,
                                 card->timestamp.tm_mday, card->timestamp.tm_mon + 1, card->timestamp.tm_year + 1900,
                                 card->timestamp.tm_hour, card->timestamp.tm_min, card->timestamp.tm_sec);

                        buf[COL_WIDTH] = '\0';
                        printf("%-*s", COL_WIDTH, buf);
                        break;

                    // colonna 2: spazio vuoto
                    case 2:
                        printf("%-*s", COL_WIDTH, ""); // vuoto
                    }
                } else {
                    printf("%-*s", COL_WIDTH, ""); // vuoto
                }
            }

            printf("\n");
        }
    }
}

void init_lavagna() {
    create_card(1, TO_DO, "Implementare integrazione per il pagamento");
    create_card(2, TO_DO, "Implementare sito web servizio");
    create_card(3, TO_DO, "Diagramma delle classi UML");
    create_card(4, TO_DO, "Studio dei requisiti dell'applicazione");
    create_card(5, TO_DO, "Realizzare CRC card");
    create_card(6, TO_DO, "Studio dei casi d'uso");
    create_card(7, TO_DO, "Realizzazione dei flow of events");
    create_card(8, TO_DO, "Diagramma di deployment");
    create_card(9, TO_DO, "Analisi delle classi");
    create_card(10, TO_DO, "Implementare testing del software");

    // mostra_lavagna();
}
