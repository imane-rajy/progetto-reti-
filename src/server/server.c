#include "server.h"
#include "../utils/log.h"
#include "lavagna.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

// numero massimo di client connessi
#define MAX_CLIENTS 30

// indirizzo del server
#define SERVER_ADDR "127.0.0.1"

// porta del server
#define SERVER_PORT 5678

// rappresenta la connessione del client al server
typedef struct {
    int sock;
    unsigned short port;
    pthread_mutex_t sock_m;
    RecvState state;
} Client;

// array di connessioni dei client al server
Client clients[MAX_CLIENTS] = {0};

// numero di client connessi al server
int num_clients = 0;

// socket di ascolto del server
int listen_sock;

// mutex per proteggere l'array di client
pthread_mutex_t server_client_m;

// mutex per proteggere l'array di utenti
pthread_mutex_t server_user_m;

// inserisce un client nell'array di client
int inserisci_client(int sock, unsigned short port) {
    pthread_mutex_lock(&server_client_m); // blocca clients

    // scansiona l'array di client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
            // è libero, inizializza qui
            clients[i].sock = sock;
            clients[i].port = port;
            clients[i].state.start = clients[i].state.end = 0;  
            pthread_mutex_init(&clients[i].sock_m, NULL);
            num_clients++;

            pthread_mutex_unlock(&server_client_m); // sblocca clients
            return 0;
        }
    }

    pthread_mutex_unlock(&server_client_m); // sblocca clients

    // se lo spazio è esaurito non si inserisce
    return -1;
}

// rimuove un client dall'array di client
void rimuovi_client(int sock) {
    pthread_mutex_lock(&server_client_m); // blocca clients

    // scansiona l'array di client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == sock) {
            // questo è il client, ripuliscilo
            clients[i].sock = 0;
            clients[i].port = 0;
            clients[i].state.start = 0;
            clients[i].state.end = 0;
            pthread_mutex_destroy(&clients[i].sock_m);
            num_clients--;

            break;
        }
    }

    pthread_mutex_unlock(&server_client_m); // sblocca clients
}

// ottiene un client a partire dal socket che lo serve
Client* get_client(int sock) {
    pthread_mutex_lock(&server_client_m); // blocca clients

    // scansiona l'array di client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == sock) {
            // questo è il client, restituisci la sua porta
            Client* client = &clients[i];

            pthread_mutex_unlock(&server_client_m);
            return client;
        }
    }

    pthread_mutex_unlock(&server_client_m); // sblocca clients
    return NULL;
}

// invia un comando ad un client identificato dalla sua porta
int send_to_client(const Command *cmd, unsigned short port) {
    pthread_mutex_lock(&server_client_m); // blocca clients

    // ottieni il client a partire dalla sua porta per ottenere socket e relativo mutex
    Client *client = NULL;

    // scansiona l'array di client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].port == port) {
            // questo è il client
            client = &clients[i];
        }
    }

    // se non è stato trovato nessun client, errore
    if (client == NULL) {
        pthread_mutex_unlock(&server_client_m); // sblocca clients
        return -1;
    }

    // invia il comando al client
    int ret = send_command(cmd, client->sock, &client->sock_m); // questa bloccherà il mutex del socket

    pthread_mutex_unlock(&server_client_m); // sblocca clients
    return ret;
}

// gestione thread

// thread che si occupa di inviare PING periodici ai client con card in DOING
void *ping_thread(void *arg __attribute__((unused))) {
    while (1) {
        sleep(1);

        // gestisci i PING
        gestisci_ping(&server_user_m); // -> lavagna.c
    }
}

// thread che si occupa di gestire le richieste dei client attraverso la select
void *select_thread(void *arg __attribute__((unused))) {
    // inizializza i set di descrittori di file
    fd_set master_set, read_set;
    int fdmax;

    // configura il set master
    FD_ZERO(&master_set);
    FD_SET(listen_sock, &master_set);
    fdmax = listen_sock;

    // configura set di lettura
    FD_ZERO(&read_set);

    // esegui ciclo di select
    while (1) {
        // copia set master nel set di lettura
        read_set = master_set;

        // scansiona con la select
        if (select(fdmax + 1, &read_set, NULL, NULL, NULL) < 0) {
            log_evento("Errore nella select\n");
            continue;
        }

        for (int i = 0; i <= fdmax; i++) {
            // controlla che ci si qualcosa da leggere
            if (!FD_ISSET(i, &read_set)) {
                continue;
            }

            if (i == listen_sock) {
                // accetta la connessione di un nuovo client
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                if (client_sock < 0) {
                    log_evento("Errore durante la accept: %s\n", strerror(errno));
                    continue;
                }

                // ottieni e registra porta del client
                unsigned short client_port = ntohs(client_addr.sin_port);
                if (inserisci_client(client_sock, client_port) < 0) {
                    close(client_sock);

                    log_evento("Errore durante la accept del client %d: spazio esaurito\n", client_port);
                    continue;
                }

                // inserisci il nuovo socket nel master set
                FD_SET(client_sock, &master_set);
                if (client_sock > fdmax) {
                    fdmax = client_sock;
                }

                log_evento("Connesso nuovo client con porta %d\n", client_port);
            } else {
                // gestisci il messaggio sul socket di un client già connesso
                int client_sock = i;

                // ottieni la porta del client
                Client* client = get_client(client_sock);

                // ricevi dal client
                Command cmd = {0};
                int ret = recv_command(&cmd, client_sock, NULL, &client->state);
                if (ret < 0) {
                    log_evento("Errore nella recv: %s\n", strerror(errno));
                    continue;
                }

                // gestisci se il socket è chiuso
                if (ret == 0) {
                    // deregistra la porta del client
                    rimuovi_client(client_sock);

                    // chiudi il socket
                    close(client_sock);

                    // aggiorna master set
                    FD_CLR(client_sock, &master_set);

                    // trova il nuovo massimo
                    for (int i = FD_SETSIZE - 1; i >= 0; i--) {
                        if (FD_ISSET(i, &master_set)) {
                            fdmax = i;
                            break;
                        }
                    }

                    log_evento("Client %d disconnesso\n", client->port);
                    continue;
                }

                // altrimenti gestisci il comando
                gestisci_comando(&cmd, client->port); // lavagna.c
            }
        }
    }
}

int main() {
    // crea socket ascolto
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_evento("Errore nella creazione del socket di ascolto: %s\n", strerror(errno));
        return -1;
    }

    // rendi il socket di ascolto riutilizzabile (per debugging più veloce)
    int yes = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // inizializza mutex
    pthread_mutex_init(&server_client_m, NULL);
    pthread_mutex_init(&server_user_m, NULL);

    // configura indirizzo server
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        printf("Indirizzo server non valido\n");
        return -1;
    }

    // collega ad indirizzo server
    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Errore nella bind");
        return -1;
    }

    // metti il socket di ascolto, in ascolto
    if (listen(listen_sock, 10) < 0) {
        perror("Errore nella listen");
        return -1;
    }

    log_evento("Server TCP in ascolto sulla porta %d...\n", SERVER_PORT);

    // inizializza lavagna
    init_lavagna();

    // inizializza thread
    pthread_t t_select, t_ping;

    pthread_create(&t_ping, NULL, ping_thread, NULL);
    pthread_create(&t_select, NULL, select_thread, NULL);

    pthread_join(t_select, NULL);
    pthread_join(t_ping, NULL);

    close(listen_sock);

    return 0;
}
