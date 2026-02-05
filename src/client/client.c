#include "../utils/card.h"
#include "../utils/command.h"
#include "../utils/log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

#define CLIENT_ADDR "127.0.0.1"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5678

#define MAX_SLEEP_TIME 5
#define MAX_CLIENTS 30

int server_sock;
int peer_sock;

struct sockaddr_in peer_addr;

pthread_mutex_t server_client_m;

volatile int registered = 0;
volatile int running = 1;

// logica di revisione

int review_ricevuti;
pthread_mutex_t review_m;
pthread_cond_t review_cond;

int send_request(unsigned short port) {
    // prepara richiesta di revisione
    Command req = {.type = REVIEW_CARD};

    // invia richiesta di revisione
    peer_addr.sin_port = htons(port);

    log_evento("Richiesta review a peer %d\n", port);
    return sendto_command(&req, peer_sock, &peer_addr);
}

int send_review(unsigned short port) {
    // prepara risposta alla revisione
    Command rev = {.type = ACK_REVIEW_CARD};

    // invia richiesta di revisione
    peer_addr.sin_port = htons(port);

    log_evento("Inviata review a peer %d\n", port);
    return sendto_command(&rev, peer_sock, &peer_addr);
}

// TODO: usare questa funzione, quindi rispondere alle richieste ed avere un modo per avere risposta alle richieste

// logica di gestione pong

int pong_lavagna() {
    // prepara pong
    Command pong = {.type = PONG_LAVAGNA};

    // invia pong
    int ret = send_command(&pong, server_sock, &server_client_m);
    if (ret < 0)
        return ret;

    log_evento("Recivuto ping, risposto pong\n");
    return 0;
}

int switch_recv(Command *cm) {
    while (1) {
        // ricevi un comando
        int ret = recv_command(cm, server_sock, NULL);

        // se c'è un errore o non è ping, restituisci
        if (ret < 0 || cm->type != PING_USER)
            return ret;

        // altrimenti rispondi
        if (pong_lavagna() < 0)
            return -1;
    }
}

// logica di gestione card

int get_user_list(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // ottieni lista
    Command cmd = {0};
    if (switch_recv(&cmd) < 0) {
        log_evento("Errore nella ricezione card: %s\n", strerror(errno));
        return -1;
    }

    if (cmd.type != SEND_USER_LIST) {
        log_evento("Ottenuto comando inaspettato (%d) per HANDLE_CARD\n", cmd.type);
        return -2;
    }

    log_evento("Ottenuta lista di utenti: ");

    // deserializza lista
    *num_clients = 0;
    for (int i = 0; i < get_argc(&cmd); i++) {
        int cl = atoi(cmd.args[i]);

        if (cl != 0) {
            // riporta client
            clients[(*num_clients)++] = cl;
            log_evento("%d ", cl);
        }
    }

    log_evento("\n");

    return 0;
}

int get_card(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // ottieni card
    Command cmd = {0};
    if (switch_recv(&cmd) < 0) {
        log_evento("Errore nella ricezione card: %s\n", strerror(errno));
        return -1;
    }

    if (cmd.type != HANDLE_CARD) {
        log_evento("Ottenuto comando inaspettato (%d) per HANDLE_CARD\n", cmd.type);
        return -2;
    }

    // deserializza card
    Card card;
    int ret = cmd_to_card(&cmd, &card);

    if (ret < 0) {
        log_evento("Impossibile deserializzare card\n");
        return -3;
    }

    // ottieni anche la lista utenti
    if (get_user_list(clients, num_clients) < 0) {
        log_evento("Impossibile ottenere lista utenti\n");
        return -4;
    }

    // fai l'ack della card
    log_evento("Invio l'ACK_CARD al server...\n");

    // prepara id
    char id_str[6];
    snprintf(id_str, 6, "%d", card.id);

    // prepara ed invia comando ack
    Command ack = {.type = ACK_CARD, .args = {id_str}};
    send_command(&ack, server_sock, &server_client_m);

    // stampa card
    log_evento("Ottenuta card:\n");

    // stampa il testo
    log_evento("%s\n", card.testo);

    // stampa i dati della card
    log_evento("ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d\n", card.id, card.client, card.timestamp.tm_mday,
               card.timestamp.tm_mon + 1, card.timestamp.tm_year + 1900, card.timestamp.tm_hour, card.timestamp.tm_min,
               card.timestamp.tm_sec);

    // restituisci solo l'indice
    return card.id;
}

int request_user_list(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // richiedi la lista utenti
    log_evento("Richiedo la lista di utenti...\n");
    Command req = {.type = REQUEST_USER_LIST};
    send_command(&req, server_sock, &server_client_m);

    // ottieni la lista utenti
    if (get_user_list(clients, num_clients) < 0) {
        log_evento("Impossibile ottenere lista utenti\n");
        return -1;
    }

    return 0;
}

int get_review(int num_clients) {
    pthread_mutex_lock(&review_m);

    while (review_ricevuti < num_clients)
        pthread_cond_wait(&review_cond, &review_m);

    pthread_mutex_unlock(&review_m);

    return 0;
}

void do_card(int card_id) {
    log_evento("Invio il CARD_DONE al server...\n");

    // prepara id
    char id_str[6];
    snprintf(id_str, 6, "%d", card_id);

    // prepara ed invia comando done
    Command ack = {.type = CARD_DONE, .args = {id_str}};
    send_command(&ack, server_sock, &server_client_m);
}

// gestione thread e socket

#define COL_WIDTH 50

void stampa_interfaccia() {
    system("clear"); // ripulisce lo schermo

    // stampa i log
    stampa_header("LOG", COL_WIDTH);
    printf("\n");
    stampa_eventi();

    // stampa la shell
    stampa_header("SHELL", COL_WIDTH);
    printf("\n");
    printf("$ ");
    fflush(stdout);
}

void *listener_thread(void *arg __attribute__((unused))) {
    while (!registered) {
    } // aspetta di essere registrato

    while (running) {
        unsigned short clients[MAX_CLIENTS]; // array per gli indici di client
        int num_clients = 0;                 // numero di client ottenuti

        // ottieni card dal server
        int id = get_card(clients, &num_clients);
        if (id < 0)
            break;

        do {
            // aspetta
            int n = rand() % MAX_SLEEP_TIME;
            sleep(n);

            // ottieni lista client
            int ret = request_user_list(clients, &num_clients);
            if (ret < 0)
                break;

            log_evento("Ottenuti %d peer\n", num_clients);
        } while (num_clients < 1);

        // richiedi review ad ogni client
        for (int i = 0; i < num_clients; i++) {
            int client = clients[i];
            log_evento("Richiedo la review dell'utente %d...\n", client);
            send_request(client);
        }

        // aspetta le review
        get_review(num_clients);

        // invia il card done
        do_card(id);

        // aggiorna interfaccia
        stampa_interfaccia();
    }

    running = 0;
    return NULL;
}

void *review_thread(void *arg __attribute__((unused))) {
    while (running) {
        Command cmd = {0};
        unsigned short port;
        int ret = recvfrom_command(&cmd, peer_sock, &port);
        if (ret < 0) {
            log_evento("Errore nella recv da peer\n");
        }

        switch (cmd.type) {
        case REVIEW_CARD:
            send_review(port);
            break;
        case ACK_REVIEW_CARD: {
            pthread_mutex_lock(&review_m);

            // questo è di interesse a get_review, segnala
            review_ricevuti++;
            pthread_cond_signal(&review_cond);

            log_evento("Ottenuta review da peer %d\n", port);
            pthread_mutex_unlock(&review_m);
            break;
        }
        default:
            log_evento("Comando inaspettato (%d) da peer %d\n", cmd.type, port);
            continue;
        }
    }

    return NULL;
}

void *console_thread(void *arg __attribute__((unused))) {
    while (running) {
        // aggiorna interfaccia
        stampa_interfaccia();

        // ottieni comando
        char buffer[BUFFER_SIZE];
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            continue;
        buffer[strcspn(buffer, "\n")] = '\0';

        // se è comand ovuoto non inviare
        if (*buffer == '\0')
            continue;

        // interpreta comando
        Command cmd = {0};
        buf_to_cmd(buffer, &cmd);

        // se di HELLO, assumi che sia registrato
        if (cmd.type == HELLO)
            registered = 1;

        // invia il comando
        if (send_command(&cmd, server_sock, &server_client_m) < 0) {
            perror("Errore nella send");
            running = 0;
            break;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // prendi argomenti
    if (argc < 2) {
        printf("Uso: ./client [numero di porta]\n");
        return 1;
    }

    // prendi porta
    unsigned short port = atoi(argv[1]);

    // inizializza rand
    srand(time(NULL) ^ port);

    // crea socket verso il server
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket verso server");
        return -1;
    }
    pthread_mutex_init(&server_client_m, NULL);

    // rendi il socket verso server riutilizzabile (per debugging più veloce)
    int yes = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // configura indirizzo client
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, CLIENT_ADDR, &client_addr.sin_addr) <= 0) {
        printf("Indirizzo client non valido\n");
        return -1;
    }

    // collega socket verso server ad indirizzo client
    if (bind(server_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Errore nella bind del socket verso server");
        return -1;
    }

    // configura indirizzo server
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        printf("Indirizzo server non valido\n");
        return -1;
    }

    // connetti al server
    if (connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione al server fallita");
        return -1;
    }

    // crea socket verso i peer
    if ((peer_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Errore nella creazione del socket verso client");
        return -1;
    }

    // rendi il socket verso i peer riutilizzabile (per debugging più veloce)
    setsockopt(peer_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // collega socket verso i peer ad indirizzo client
    if (bind(peer_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Errore nella bind del socket verso i peer");
        return -1;
    }

    // configura indirizzo peer
    peer_addr.sin_family = AF_INET;
    // la porta verrà riempita dopo

    if (inet_pton(AF_INET, CLIENT_ADDR, &peer_addr.sin_addr) <= 0) {
        printf("Indirizzo peer non valido\n");
        return -1;
    }

    // inizializza mutex e condition variable per review
    review_ricevuti = 0;
    pthread_mutex_init(&review_m, NULL);
    pthread_cond_init(&review_cond, NULL);

    // inizializza thread
    pthread_t t_listener, t_console, t_review;

    // thread di ascolto, gestisce il ciclo delle card
    pthread_create(&t_listener, NULL, listener_thread, NULL);

    // thread console, gestisce i comandi da tastiera
    pthread_create(&t_console, NULL, console_thread, NULL);

    // thread review, gestisce le review fra peer
    pthread_create(&t_review, NULL, review_thread, NULL);

    pthread_join(t_listener, NULL);
    pthread_join(t_console, NULL);
    pthread_join(t_review, NULL);

    close(server_sock);

    return 0;
}
