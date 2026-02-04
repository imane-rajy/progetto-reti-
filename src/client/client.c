#include "../includes.h"
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
srand(time(NULL));
int client_sock;

pthread_mutex_t client_sock_m;

volatile int running = 1;

void get_card(){

    char buffer[BUFFER_SIZE];
    Command cmd = {0};
    if(recv_command(&cmd, buffer, client_sock, &client_sock_m) < 0 ){
        perror("Errore nella receive");
        running = 0;
        break;
    }

    if(cmd.type == HANDLE_CARD && cmd.args){
        
    }
}

void *listener_thread(void *arg) {
   

    while (running) {
        // effettua il ciclo della card:
        // - ottieni card dal server
        Command cmd = {0};
        get_card();
        if(recv_command(&cmd, buffer, client_sock, &client_sock_m) < 0 ){
            perror("Errore nella receive");
            running = 0;
            break;
        }

        
        // - aspetta
        int n = rand() % 30;
        sleep(n);
        // - ottieni lista client
        Command cmd = { .type = REQUEST_USER_LIST};
        if (send_command(&cmd, client_sock, &client_sock_m) < 0) {
            perror("Errore nella send");
            running = 0;
            break;
        }

        if (recv_command(&cmd, buffer, client_sock, &client_sock_m) < 0) {
            perror("Errore nella receive");
            running = 0;
            break;
        }

        printf()
        // - richiedi review ad ogni client
        // - fai ack della card
    }

    return NULL;
}

void *console_thread(void *arg) {
    char buffer[BUFFER_SIZE];

    while (running) {
        printf("$ ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            continue;
        buffer[strcspn(buffer, "\n")] = '\0';

        Command cmd = {0};
        buf_to_cmd(buffer, &cmd);

        if (send_command(&cmd, client_sock, &client_sock_m) < 0) {
            perror("Errore nella send");
            running = 0;
            break;
        }

        // if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
        //     perror("Errore nella send");
        //     running = 0;
        //     break;
        // }
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
