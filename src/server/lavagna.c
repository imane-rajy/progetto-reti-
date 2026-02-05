#include "lavagna.h"
#include "../utils/log.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// numero massimo di card presenti nel sistema
#define MAX_CARDS 100

// numero massimo di utenti registrati
#define MAX_USERS 30

// numero di porta da cui partono gli utenti
#define MIN_PORT_USERS 5679

// numero minimo di utenti necessari per fare CARD_DONE
#define MIN_NUM_USERS_DONE 2

// tempo da aspettare prima di inviare il PING
#define WAIT_TO_PING 30

// tempo da aspettare prima di deregestrare un utente che non ha fatto PONG
#define WAIT_FOR_PONG 10

// gestione degli user

// rappresenta lo stato di un utente
typedef enum { BUSY, IDLE, ASSIGNED_CARD } UserState;

// rappresenta un utente, il suo stato di gestione delle card e dei PING
typedef struct {
    unsigned short port;
    UserState state;
    int card_id;
    int timer_ping;
    int ping_sent;
} User;

// array di utenti registrati alla lavagna
User users[MAX_USERS] = {0};

// numero di utenti registrati alla lavagna
int num_users = 0;

// macro per ottenere l'indice di utente nell'array di utenti a partire dal numero di porta
#define USER_IDX(port) ((port) - MIN_PORT_USERS)

// inserisce un utente nell'array di utenti
int inserisci_user(unsigned short port) {
    // controlla che la porta sia nel range ammissibile
    if (port < MIN_PORT_USERS || port >= MIN_PORT_USERS + MAX_USERS) {
        return -1;
    }

    // ottieni l'indice dell'utente
    int idx = USER_IDX(port);

    // controlla che sia vuoto
    if (users[idx].port != 0) {
        return -2;
    }

    // imposta l'utente
    users[idx].port = port;
    users[idx].timer_ping = WAIT_TO_PING;
    users[idx].ping_sent = 0;
    num_users++;

    // restituisci l'indice
    return idx;
}

User *controlla_user(unsigned short client) {
    int idx = client - MIN_PORT_USERS;
    User *user = &users[idx];
    if (user->port == client)
        return user;

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

// logica di gestione card

Card cards[MAX_CARDS] = {0};
int num_cards = 0;

int request_user_list(User *user) {
    // prepara buffer per id client
    char client_ports[num_users][6];

    // prepara risposta
    Command cm = {
        .type = SEND_USER_LIST
        // verrà popolato in seguito
    };

    log_evento("Inviata la lista dei client: ");

    // itera sui client
    int n = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (user == &users[i]) {
            continue;
        }

        if (users[i].port != 0) {
            // copia id client nel buffer
            snprintf(client_ports[n], 6, "%d", users[i].port);
            log_evento("%d ", users[i].port);

            // usa come argomento
            cm.args[n] = client_ports[n];
            n++;
        }
    }

    log_evento("a %d\n", user->port);

    // invia risposta
    send_to_client(&cm, user->port);

    return 0;
}

void handle_card(User *user) {
    for (int i = 0; i < MAX_CARDS; i++) {
        // solo se esiste
        if (cards[i].id == 0)
            continue;

        // solo se è in TO_DO
        if (cards[i].colonna != TO_DO)
            continue;

        // solo non è già assegnata
        if (cards[i].client != 0)
            continue;

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
        send_to_client(&cmd, user->port);

        // invia lista utenti
        request_user_list(user);

        log_evento("Assegnata card %d a client %d\n", card->id, user->port);
        return;
    }
}

void handle_cards() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].state == IDLE)
            handle_card(&users[i]);
    }
}

int create_card(int id, Colonna colonna, const char *testo) {
    // verifica che l'id sia unico
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].id == id) {
            return -1;
        }
    }

    // alloca la card
    int idx = num_cards;
    num_cards++;
    if (idx >= MAX_CARDS) {
        return -2;
    }

    // imposta i dati della card
    cards[idx].id = id;
    cards[idx].colonna = colonna;

    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';

    log_evento("Creata card %d: %s\n", id, testo);

    handle_cards(); // fai il push della card
    return idx;
}

int hello(unsigned short client) {
    // prova a registrate un utente
    int idx = inserisci_user(client);

    if (idx >= 0) {
        log_evento("Registrato client %d\n", client);
        handle_card(&users[idx]);
        return 0;
    }

    return -1;
}

int find_card(int card_id) {
    // scansiona le card
    for (int i = 0; i < MAX_CARDS; i++) {
        // restituisci l'indice dato l'id
        if (cards[i].id == card_id)
            return i;
    }

    return -1;
}

int move_card(int card_id, Colonna to) {
    // trova l'indice della card
    int idx = find_card(card_id);
    if (idx < 0) {
        return -1;
    }

    // assicurati che non sia già li
    if (cards[idx].colonna == to) {
        return -2;
    }

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
        if (idx < 0) {
            return -1;
        }

        Card *card = &cards[idx];

        // annulla il suo utente
        card->client = 0;

        // mettila in TO_DO
        if (card->colonna == DOING) {
            move_card(card->id, TO_DO);
            handle_cards(); // fai il push della card
        }

        log_evento("Deregistrato client %d\n", port);
        return 0;
    }

    return -1;
}

int ack_card(User *user, int card_id) {
    // controlla che sia stata assegnata una card
    if (user->state != ASSIGNED_CARD)
        return -1;

    // ottieni la card
    int idx = find_card(user->card_id);

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) {
        return -1;
    }

    // controlla che la card siain TO_DO
    if (cards[idx].colonna != TO_DO) {
        return -1;
    }

    // aggiorna lo stato della card e dell'utente
    cards[idx].colonna = DOING;
    user->state = BUSY;

    log_evento("Ricevuto ACK per card %d\n", card_id);
    return 0;
}

int card_done(User *user, int card_id) {
    // controlla che ci sia il numero minimo di utenti registrati
    if (num_users < MIN_NUM_USERS_DONE) {
        return -1;
    }

    // controlla che stia gestendo una card
    if (user->state != BUSY) {
        return -2;
    }

    // ottieni la card
    int idx = find_card(user->card_id);

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) {
        return -3;
    }

    // controlla che la card sia in DOING
    if (cards[idx].colonna != DOING) {
        return -4;
    }

    // aggiorna lo stato della card e dell'utente
    cards[idx].colonna = DONE;
    user->state = IDLE;
    timestamp_card(&cards[idx]);

    // invia un altra card
    handle_card(user);

    log_evento("Ricevuto CARD_DONE per card %d\n", card_id);
    return 0;
}

// logica di gestione ping

int pong_lavagna(User *user) {
    user->ping_sent = 0;
    user->timer_ping = WAIT_TO_PING;

    log_evento("Ricevuto PONG da utente, tutto ok\n");
    return 0;
}

int gestisci_ping(pthread_mutex_t *server_user_m) {
    pthread_mutex_lock(server_user_m);

    for (int i = 0; i < MAX_USERS; i++) {
        // scansiona tutti gli utenti
        User *user = &users[i];

        if (user->port != 0 && user->state == BUSY) {
            // solo se registrato e ha una card in DOING

            if (!--user->timer_ping) {
                // scatta il timer
                if (!user->ping_sent) {
                    // ping non già inviato
                    Command cmd = {.type = PING_USER};
                    send_to_client(&cmd, user->port);
                    user->ping_sent = 1;
                    user->timer_ping = WAIT_FOR_PONG;

                    log_evento("Inviato PING a utente\n");
                } else {
                    // ping già inviato, deregistra
                    quit(user);

                    log_evento("Utente non ha risposto a PING, lo deregistro\n");
                }
            }
        }
    }

    pthread_mutex_unlock(server_user_m);
    return 0;
}

#define COL_WIDTH 60

void stampa_lavagna() {
    system("clear");

    // stampa header
    for (int c = 0; c < NUM_COLS; c++) {
        stampa_header(col_names[c], COL_WIDTH);
    }
    printf("\n");

    // ottieni il numero di righe
    int max_rows = 0;
    int col_counts[NUM_COLS] = {0};
    for (int i = 0; i < num_cards; i++) {
        col_counts[cards[i].colonna]++;
        if (col_counts[cards[i].colonna] > max_rows)
            max_rows = col_counts[cards[i].colonna];
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
                        snprintf(buf, sizeof(buf), "ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d", card->id,
                                 card->client, card->timestamp.tm_mday, card->timestamp.tm_mon + 1,
                                 card->timestamp.tm_year + 1900, card->timestamp.tm_hour, card->timestamp.tm_min,
                                 card->timestamp.tm_sec);

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

    // stampa log
    stampa_header("LOG", COL_WIDTH * 3);
    printf("\n");
    stampa_eventi();
}

void gestisci_comando(const Command *cmd, unsigned short port) {
    // controlla che si registrato o che si stia registrando adesso
    User *user = controlla_user(port);

    if (user == NULL && cmd->type != HELLO) {
        log_evento("Ottenuto comando non HELLO (%d) da client non registrato %d\n", cmd->type, port);
        return;
    }

    int ret = -1;

    switch (cmd->type) {
    case CREATE_CARD: {
        int id = atoi(cmd->args[0]);
        Colonna colonna = str_to_col(cmd->args[1]);

        // copia tutti gli ultimi argomenti nel testo
        char testo[MAX_TESTO];
        char *pun = testo;
        int argc = get_argc(cmd);

        for (int i = 2; i < argc; i++) {
            const char *arg = cmd->args[i];
            size_t len = strlen(arg);

            // nel caso di overflow esci
            if ((pun - testo) + len >= MAX_TESTO - 1)
                break;

            strcpy(pun, arg);
            pun += len;

            if (i != argc - 1 && (pun - testo) < MAX_TESTO - 1) {
                *pun++ = ' ';
            }
        }

        // termina
        *pun = '\0';

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
        ret = pong_lavagna(user);
        break;
    }
    case ACK_CARD: {
        if (get_argc(cmd) < 1)
            break;

        int card_id = atoi(cmd->args[0]);
        ret = ack_card(user, card_id);
        break;
    }
    case REQUEST_USER_LIST: {
        ret = request_user_list(user);
        break;
    }
    case CARD_DONE: {
        if (get_argc(cmd) < 1)
            break;

        int card_id = atoi(cmd->args[0]);
        ret = card_done(user, card_id);
        break;
    }
    default:
        break;
    }

    if (ret < 0) {
        log_evento("Errore nell'esecuzione del comando %d del client %d\n", cmd->type, port);
    }

    // aggiorna interfaccia
    stampa_lavagna();
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

    stampa_lavagna();
}
