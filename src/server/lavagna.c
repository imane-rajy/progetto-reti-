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

// stringa che contiene l'errore verificato
const char *errore;

// gestione degli utenti

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
        errore = "porta richiesta fuori dal range ammissibile";
        return -1;
    }

    // ottieni l'indice dell'utente
    int idx = USER_IDX(port);

    // controlla che sia vuoto
    if (users[idx].port != 0) {
        errore = "utente da registrare già registrato";
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

// controlla che l'utente sia registrato o che si stia registrando
User *controlla_user(unsigned short port) {
    // ottieni l'indice dell'utente
    int idx = USER_IDX(port);

    // verifica che la porta corrisponda
    User *user = &users[idx];
    if (user->port == port) {
        return user;
    }

    errore = "utente richiesto non registrato";
    return NULL;
}

// rimuove un utente già registrato
int rimuovi_user(User *user) {
    // solo se non NULL
    if (user != NULL) {
        user->port = 0;
        num_users--;
        return 0;
    }

    errore = "utente da deregistrare non registrato";
    return -1;
}

// logica di gestione card

// array delle card
Card cards[MAX_CARDS] = {0};

// numero di card presenti nel sistema
int num_cards = 0;

// trova l'indice di una card a partire dal suo id
int find_card(int card_id) {
    // scansiona le card
    for (int i = 0; i < MAX_CARDS; i++) {
        // restituisci l'indice dato l'id
        if (cards[i].id == card_id) {
            return i;
        }
    }

    errore = "card con indice richiesto inesistente";
    return -1;
}

// sposta una card, dato l'id, ad una certa colonna
int move_card(int card_id, Colonna to) {
    // trova l'indice della card
    int idx = find_card(card_id);
    if (idx < 0) {
        return -1;
    }

    // assicurati che non sia già li
    if (cards[idx].colonna == to) {
        errore = "card giù situata nella colonna richiesta";
        return -2;
    }

    // sposta la card
    cards[idx].colonna = to;

    return 0;
}

// handler dei comandi

// invia la lista degli utenti all'utente (sé stesso non compreso)
int request_user_list(User *user) {
    // prepara buffer per id client
    char client_ports[num_users][6]; // 2^16 = 65535, 5 caratteri, 6 con il terminatore

    // prepara risposta
    Command cmd = {
        .type = SEND_USER_LIST
        // verrà popolato in seguito
    };

    log_evento("Inviata la lista dei client: ");

    // itera sugli utenti
    int n = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (user == &users[i]) {
            continue;
        }

        if (users[i].port != 0) {
            // copia id client nel buffer
            snprintf(client_ports[n], 6, "%d", users[i].port);

            // usa come argomento
            cmd.args[n] = client_ports[n];
            n++;

            log_evento("%d ", users[i].port);
        }
    }

    log_evento("a %d\n", user->port);

    // invia risposta
    send_to_client(&cmd, user->port);

    return 0;
}

// invia, se esiste, la prossima card in TO_DO all'utente
void handle_card(User *user) {
    // scansiona tutte le card
    for (int i = 0; i < MAX_CARDS; i++) {
        // solo se esiste
        if (cards[i].id == 0) {
            continue;
        }

        // solo se è in TO_DO
        if (cards[i].colonna != TO_DO) {
            continue;
        }

        // solo non è già assegnata
        if (cards[i].port != 0) {
            continue;
        }

        // prendi la card
        Card *card = &cards[i];

        // assegnala al client
        card->port = user->port;
        user->card_id = card->id;
        user->state = ASSIGNED_CARD;
        timestamp_card(card);

        // nota: la card resta in TO_DO, e lo stato dell'utente passa a ASSIGNED_CARD, la card passera in DOING quando
        // si riceverà l'ACK_CARD (e lo stato dell'utente passerà a BUSY)

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

// invia, finché ce ne sono, le card che sono in TO_DO agli utenti che sono in IDLE, una per ogni utente
void handle_cards() {
    // scansiona gli utenti
    for (int i = 0; i < MAX_USERS; i++) {
        // se è in IDLE, usa handle_card
        if (users[i].state == IDLE) {
            handle_card(&users[i]);
        }
    }
}

// crea un nuova card a partire da id, colonna e testo
int create_card(int id, Colonna colonna, const char *testo) {
    // controlla che l'id sia unico
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].id == id) {
            errore = "id della card da creare già usato";
            return -1;
        }
    }

    // alloca la card
    int idx = num_cards;
    num_cards++;

    // controlla che lo spazio non sia stato esaurito
    if (idx >= MAX_CARDS) {
        errore = "spazio esaurito per le card";
        return -2;
    }

    // imposta i dati della card
    cards[idx].id = id;
    cards[idx].colonna = colonna;
    timestamp_card(&cards[idx]);

    // copia il testo nella card
    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';

    log_evento("Creata card %d: %s\n", id, testo);

    // fai il push della card
    handle_cards();
    return idx;
}

// registra un utente a partire dal suo numero di porta
int hello(unsigned short port) {
    // prova a registrare un utente
    int idx = inserisci_user(port);

    if (idx >= 0) {
        log_evento("Registrato utente %d\n", port);
        handle_card(&users[idx]);
        return 0;
    }

    return -1;
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
        card->port = 0;

        // se era in DOING rimettila in TO_DO
        if (card->colonna == DOING) {
            card->colonna = TO_DO;

            // fai il push della card
            handle_cards();
        }

        log_evento("Deregistrato client %d\n", port);
        return 0;
    }

    return -1;
}

// gestisce l'ACK_CARD su una card
int ack_card(User *user, int card_id) {
    // controlla che sia stata assegnata una card
    if (user->state != ASSIGNED_CARD) {
        errore = "l'utente che ha fatto ACK_CARD non ha ricevuto nessuna card";
        return -1;
    }

    // ottieni la card
    int idx = find_card(user->card_id);
    if (idx < 0) {
        return -2;
    }

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) {
        errore = "l'utente ha fatto ACK_CARD su una card non posseduta";
        return -3;
    }

    // controlla che la card siain TO_DO
    if (cards[idx].colonna != TO_DO) {
        errore = "ACK_CARD su una card non in TO_DO";
        return -4;
    }

    // aggiorna lo stato della card e dell'utente
    cards[idx].colonna = DOING;
    user->state = BUSY;

    log_evento("Ricevuto ACK per card %d\n", card_id);
    return 0;
}

// gestisce il CARD_DONE su una card
int card_done(User *user, int card_id) {
    // controlla che ci sia il numero minimo di utenti registrati
    if (num_users < MIN_NUM_USERS_DONE) {
        errore = "non ci sono abbastanza utenti per fare CARD_DONE";
        return -1;
    }

    // controlla che stia gestendo una card
    if (user->state != BUSY) {
        errore = "l'utente che ha fatto CARD_DONE non aveva una card in DOING";
        return -2;
    }

    // ottieni la card
    int idx = find_card(user->card_id);
    if (idx < 0) {
        return -3;
    }

    // controlla che l'id corrisponda
    if (cards[idx].id != card_id) {
        errore = "l'utente ha fatto CARD_DONE su una card non posseduta";
        return -4;
    }

    // controlla che la card sia in DOING
    if (cards[idx].colonna != DOING) {
        errore = "CARD_DONE su una card non in DOING";
        return -5;
    }

    // aggiorna lo stato della card e dell'utente
    user->state = IDLE;
    cards[idx].colonna = DONE;
    timestamp_card(&cards[idx]);

    // invia un altra card
    handle_card(user);

    log_evento("Ricevuto CARD_DONE per card %d\n", card_id);
    return 0;
}

// logica di gestione PING

// gestisce il PONG ricevuto da un utente
int pong_lavagna(User *user) {
    user->ping_sent = 0;
    user->timer_ping = WAIT_TO_PING;

    log_evento("Ricevuto PONG da utente, tutto ok\n");
    return 0;
}

// chiamata ogni secondo per gestire i PING
int gestisci_ping(pthread_mutex_t *server_user_m) {
    pthread_mutex_lock(server_user_m); // blocca users

    // scansiona tutti gli utenti
    for (int i = 0; i < MAX_USERS; i++) {
        User *user = &users[i];

        // solo se registrato e ha una card in DOING
        if (user->port != 0 && user->state == BUSY) {
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
                    // PING già inviato, deregistra
                    quit(user);

                    log_evento("Utente non ha risposto a PING, lo deregistro\n");
                }
            }
        }
    }

    pthread_mutex_unlock(server_user_m); // sblocca users
    return 0;
}

// id della lavagna corrente
#define ID_LAVAGNA 0

// larghezza di una colonna della lavagna (60 per evitare overflow)
#define COL_WIDTH 60

// stampa la lavagna a schermo, assieme alla lista degli ultimi eventi
void stampa_lavagna() {
    // ripulisci lo schermo
    system("clear");

    // stampa header della lavagna
    printf("ID Lavagna: %d, Utenti registrati: %d, Card create: %d\n", ID_LAVAGNA, num_users, num_cards);

    // stampa header colonne
    for (int c = 0; c < NUM_COLS; c++) {
        stampa_header(col_to_str(c), COL_WIDTH);
    }
    printf("\n");

    // ottieni il numero di righe
    int max_rows = 0;

    int col_counts[NUM_COLS] = {0};
    for (int i = 0; i < num_cards; i++) {
        col_counts[cards[i].colonna]++;
        if (col_counts[cards[i].colonna] > max_rows) {
            max_rows = col_counts[cards[i].colonna];
        }
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

        // 3 righe per card
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < NUM_COLS; c++) {
                if (row < col_index[c] && r < 2) {
                    // ottieni la card
                    Card *card = col_cards[c][row];

                    switch (r) {
                        // riga 0: testo
                        case 0: {
                            // buffer per testo
                            char buf[COL_WIDTH + 1];

                            // scrivi testo nel buffer
                            strncpy(buf, card->testo, COL_WIDTH);

                            // stampa buffer
                            printf("%-*s", COL_WIDTH, buf);
                            break;
                        }
                        // riga 1: altri dati
                        case 1: {
                            // buffer per timestamp
                            char timebuf[32];
                            // buffer per dati
                            char buf[COL_WIDTH + 1];

                            // scrivi timestamp nel buffer
                            strftime(timebuf, sizeof(timebuf), "%d-%m-%Y %H:%M:%S", &card->timestamp);
                            // scrivi dati nel buffer
                            snprintf(buf, sizeof(buf), "ID: %d Client: %d %s", card->id, card->port, timebuf);

                            // stampa buffer
                            printf("%-*s", COL_WIDTH, buf);
                            break;
                        }
                    }
                } else {
                    // vuoto
                    printf("%-*s", COL_WIDTH, "");
                }
            }

            printf("\n");
        }
    }

    // stampa gli eventi
    stampa_header("LOG", COL_WIDTH * 3);
    printf("\n");
    stampa_eventi();
}

// gestisce un comando inviato da un certo client
void gestisci_comando(const Command *cmd, unsigned short port) {
    // ottieni l'utente che ha inviato il comando
    User *user = controlla_user(port);

    // controlla che sia registrato o che si stia registrando adesso
    if (user == NULL && cmd->type != HELLO) {
        log_evento("Ottenuto comando %s da client non registrato %d\n", cmdtype_to_str(cmd->type), port);

        // aggiorna interfaccia
        stampa_lavagna();
        return;
    }

    // conterrà il valore di ritorno
    int ret;

    switch (cmd->type) {
        case CREATE_CARD: {
            if (get_num_args(cmd) < 3) {
                break;
            }

            // ottieni argomenti
            int id = atoi(cmd->args[0]);
            Colonna colonna = str_to_col(cmd->args[1]);

            // copia tutti gli ultimi argomenti nel testo
            char testo[MAX_TESTO];
            char *pun = testo;
            int argc = get_num_args(cmd);

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

            // termina il testo
            *pun = '\0';

            ret = create_card(id, colonna, testo);
            break;
        }
        case HELLO: {
            ret = hello(port); // port e non user perché non è ancora registrato
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
            if (get_num_args(cmd) < 1) {
                break;
            }

            // ottieni argomenti
            int card_id = atoi(cmd->args[0]);

            ret = ack_card(user, card_id);
            break;
        }
        case REQUEST_USER_LIST: {
            ret = request_user_list(user);
            break;
        }
        case CARD_DONE: {
            if (get_num_args(cmd) < 1) {
                break;
            }

            // ottieni argomenti
            int card_id = atoi(cmd->args[0]);

            ret = card_done(user, card_id);
            break;
        }
        default: {
            ret = -1;
            errore = "comando inesistente";
            break;
        }
    }

    // se c'è stato errore riportalo
    if (ret < 0) {
        log_evento("Errore nell'esecuzione del comando %s del client %d: %s\n", cmdtype_to_str(cmd->type), port,
                   errore);
    }

    // aggiorna interfaccia
    stampa_lavagna();
}

// inizializza la lavagna con le prime 10 card
void init_lavagna() {
    // crea le prime 10 card
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

    // aggiorna interfaccia
    stampa_lavagna();
}
