#include "server.h"
#include "lavagna.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5678

typedef struct {
    int socket;
    unsigned short port;
} Client;

Client clients[MAX_CLIENTS] = {0};
int num_client = 0;

int listen_sock; 

pthread_mutex_t server_sock_m;
pthread_mutex_t server_user_m;

void inserisci_client(int sock, unsigned short port) {
    pthread_mutex_lock(&server_sock_m);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0){
            clients[i].socket = sock;
            clients[i].port = port;
            break;
        }
    }

    // errore: spazio esaurito
    pthread_mutex_unlock(&server_sock_m);
}

void rimuovi_client(int sock) {
    pthread_mutex_lock(&server_sock_m);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sock) {
            clients[i].socket = 0;
            clients[i].port = 0;
            break;
        }
    }

    pthread_mutex_unlock(&server_sock_m);
}

unsigned short get_port(int sock) {
    pthread_mutex_lock(&server_sock_m);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sock) {
            unsigned short port = clients[i].port;
            pthread_mutex_unlock(&server_sock_m);
            return port;
        }
    }

    pthread_mutex_unlock(&server_sock_m);
    return 0;
}

int get_socket(unsigned short port) {
    pthread_mutex_lock(&server_sock_m);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].port == port) {
            int sock = clients[i].socket;
            pthread_mutex_unlock(&server_sock_m);
            return sock;
        }
    }

    pthread_mutex_unlock(&server_sock_m);
    return 0;
}

int send_client(const Command *cm, unsigned short port) { 
    int sock = get_socket(port);
    if (sock == 0) return -1;

    int ret = send_command(cm, sock, &server_sock_m);

    return ret;
}

void * ping_thread(void *arg __attribute__((unused))){
    while(1){
        sleep(1);
        gestici_ping(&server_user_m);
    }
}

void *select_thread(void *arg __attribute__((unused))){ 
    // inizializza multiplexing
    fd_set master_set, read_set;
    int fdmax;

    FD_ZERO(&master_set);

    // inserisci il socket di ascolto nel set master
    FD_SET(listen_sock, &master_set);
    fdmax = listen_sock;

    // configura set di lettura
    FD_ZERO(&read_set);

    // esegui ciclo di multiplexing
    while (1) {
        // copia set master nel set di ascolto
        read_set = master_set;

        // scansiona con la select
        if (select(fdmax + 1, &read_set, NULL, NULL, NULL) < 0) {
			printf("Errore nella select\n");
			continue;
		}

        for (int i = 0; i <= fdmax; i++) {
            // controlla che si qualcosa da leggere
            if (!FD_ISSET(i, &read_set)) { continue; }

            if (i == listen_sock) {
                // accetta nuovo client
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                if (client_sock < 0) {
                    perror("Errore durante la accept");
                    continue;
                }

                // inserisci nel master set
                FD_SET(client_sock, &master_set);
                if (client_sock > fdmax) { fdmax = client_sock; }

                // ottieni e registra porta
                unsigned short client_port = ntohs(client_addr.sin_port);
                inserisci_client(client_sock, client_port);

                printf("Connesso nuovo client con porta %d\n", client_port);
            } else {
                // gestisci client
                int client_sock = i;

                // ottieni porta client
                unsigned short client_port = get_port(client_sock);

                // ricevi dal client
                Command cmd = {0};
                int ret = recv_command(&cmd, client_sock, NULL);
                if (ret < 0) {
                    perror("Errore nella recv");
                    continue;
                }

                // gestisci socket chiusi
                if (ret == 0) {
                    rimuovi_client(client_sock);

                    // aggiorna master set
                    FD_CLR(client_sock, &master_set);

                    for (int i = FD_SETSIZE - 1; i >= 0; i--) {
                        if (FD_ISSET(i, &master_set)) {
                            fdmax = i;
                            break;
                        }
                    }

                    printf("Client %d disconnesso\n", client_port);
                    continue;
                }

                gestisci_comando(&cmd, client_port);
            }
        }
    }


}

int main() {
    // crea socket ascolto
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Errore nella creazione del socket di ascolto");
        return -1;
    }

    // rendi il socket di ascolto riutilizzabile (per debugging più veloce)
    int yes = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // inizializza mutex
    pthread_mutex_init(&server_sock_m, NULL);
    pthread_mutex_init(&server_user_m, NULL);
    // configura indirizzo server
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);

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

    printf("Server TCP in ascolto sulla porta %d...\n", SERVER_PORT);

    // inizializza lavagna
    init_lavagna();

    pthread_t t_select, t_ping;

    // thread che gestisce i client con la select 
    pthread_create(&t_select, NULL, select_thread, NULL);

    // thread che gestisce i ping
    pthread_create(&t_ping, NULL, ping_thread, NULL);

    pthread_join(t_select, NULL);
    pthread_join(t_ping, NULL);

	close(listen_sock);
 
    return 0;
}
