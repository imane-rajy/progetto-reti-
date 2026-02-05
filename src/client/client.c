#include "../card.h"
#include "../command.h"
#include <arpa/inet.h>
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

int client_sock;

pthread_mutex_t client_sock_m;

volatile int registered = 0;
volatile int running = 1;

int pong_lavagna() {
    // prepara pong
    Command pong = {.type = PONG_LAVAGNA};

    // invia pong
    int ret = send_command(&pong, client_sock, &client_sock_m);
    if (ret < 0) return ret;

    printf("Inviato pong\n");
    return 0;
}

int switch_recv(Command *cm) {
    while (1) {
        // ricevi un comando
        int ret = recv_command(cm, client_sock, &client_sock_m);

        // se c'è un errore o non è ping, restituisci
        if (ret < 0 || cm->type != PING_USER) return ret;

        // altrimenti rispondi
        if (pong_lavagna() < 0) return -1;
    }
}

int get_user_list(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // ottieni lista
    Command cmd = {0};
    if (switch_recv(&cmd) < 0) {
        perror("Errore nella ricezione card");
        return -1;
    }

    if (cmd.type != SEND_USER_LIST) {
        printf("Ottenuto comando inaspettato (%d) per HANDLE_CARD\n", cmd.type);
        return -2;
    }

    printf("Ottenuta lista di utenti: ");

    // deserializza lista
    *num_clients = 0;
    for (int i = 0; i < get_argc(&cmd); i++) {
        int cl = atoi(cmd.args[i]);

        if (cl != 0) {
            // riporta client
            clients[(*num_clients)++] = cl;
            printf("%d ", cl);
        }
    }

    printf("\n");

    return 0;
}

int get_card(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // ottieni card
    Command cmd = {0};
    if (switch_recv(&cmd) < 0) {
        perror("Errore nella ricezione card");
        return -1;
    }

    if (cmd.type != HANDLE_CARD) {
        printf("Ottenuto comando inaspettato (%d) per HANDLE_CARD\n", cmd.type);
        return -2;
    }

    // deserializza card
    Card card;
    int ret = cmd_to_card(&cmd, &card);

    if (ret < 0) {
        printf("Impossibile deserializzare card\n");
        return -3;
    }

    // ottieni anche la lista utenti
    if (get_user_list(clients, num_clients) < 0) {
        printf("Impossibile ottenere lista utenti\n");
        return -4;
    }

    // fai l'ack della card
    printf("Invio l'ACK_CARD al server...\n");

    // prepara id
    char id_str[6];
    snprintf(id_str, 6, "%d", card.id);

    // prepara ed invia comando ack
    Command ack = {.type = ACK_CARD, .args = {id_str}};
    send_command(&ack, client_sock, &client_sock_m);

    // stampa card
    printf("Ottenuta card:\n");

    // stampa il testo
    printf("%s\n", card.testo);

    // imposta buffer per i dati della card
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d", card.id, card.client, card.timestamp.tm_mday,
             card.timestamp.tm_mon + 1, card.timestamp.tm_year + 1900, card.timestamp.tm_hour, card.timestamp.tm_min,
             card.timestamp.tm_sec);

    // stampa i dati della card
    printf("%s\n", buffer);

    // restituisci solo l'indice
    return card.id;
}

int request_user_list(unsigned short clients[MAX_CLIENTS], int *num_clients) {
    // richiedi la lista utenti
    printf("Richiedo la lista di utenti...\n");
    Command req = {.type = REQUEST_USER_LIST};
    send_command(&req, client_sock, &client_sock_m);

    // ottieni la lista utenti
    if (get_user_list(clients, num_clients) < 0) {
        printf("Impossibile ottenere lista utenti\n");
        return -1;
    }

    return 0;
}

int get_review(unsigned short client) {
    // TODO: implementa
    printf("Richiedo la review dell'utente %d...\n", client);
    return 1; // per adesso dai sempre una valutazione positiva
}

void do_card(int card_id) {
    printf("Invio il CARD_DONE al server...\n");

    // prepara id
    char id_str[6];
    snprintf(id_str, 6, "%d", card_id);

    // prepara ed invia comando done
    Command ack = {.type = CARD_DONE, .args = {id_str}};
    send_command(&ack, client_sock, &client_sock_m);
}

void *listener_thread(void *arg __attribute__((unused))) {
    while (!registered) {}

    while (running) {
        unsigned short clients[MAX_CLIENTS]; // array per gli indici di client
        int num_clients;                     // numero di client ottenuti

        // ottieni card dal server
        int id = get_card(clients, &num_clients);
        if (id < 0) break;

        // aspetta
        int n = rand() % MAX_SLEEP_TIME;
        sleep(n);

        // ottieni lista client
        int ret = request_user_list(clients, &num_clients);
        if (ret < 0) break;

        // richiedi review ad ogni client
        for (int i = 0; i < num_clients; i++) {
            int rev = get_review(clients[i]);
            if (rev < 0) break;
        }

        // invia il card done
        do_card(id);
    }

    running = 0;
    return NULL;
}

void *console_thread(void *arg __attribute__((unused))) {
    char buffer[BUFFER_SIZE];

    while (running) {
        printf("$ ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) continue;
        buffer[strcspn(buffer, "\n")] = '\0';

        Command cmd = {0};
        buf_to_cmd(buffer, &cmd);

        if (cmd.type == HELLO) registered = 1;

        if (send_command(&cmd, client_sock, &client_sock_m) < 0) {
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

    // crea socket
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket");
        return -1;
    }
    pthread_mutex_init(&client_sock_m, NULL);

    // rendi il socket di ascolto riutilizzabile (per debugging più veloce)
    int yes = 1;
    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // configura indirizzo client
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, CLIENT_ADDR, &client_addr.sin_addr) <= 0) {
        printf("Indirizzo client non valido\n");
        return -1;
    }

    // collega ad indirizzo client
    if (bind(client_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Errore nella bind");
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
    if (connect(client_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        return -1;
    }

    // inizializza thread
    pthread_t t_listener, t_console;

    // thread di ascolto, gestisce il ciclo delle card
    pthread_create(&t_listener, NULL, listener_thread, NULL);

    // thread console, gestisce i comandi da tastiera
    pthread_create(&t_console, NULL, console_thread, NULL);

    pthread_join(t_listener, NULL);
    pthread_join(t_console, NULL);
    close(client_sock);

    return 0;
}
