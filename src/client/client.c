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

int client_sock;

pthread_mutex_t client_sock_m;

volatile int registered = 0;
volatile int running = 1;

int get_card() {
	// ottieni card 
	Command cmd = {0};
    if (recv_command(&cmd, client_sock, &client_sock_m) < 0) {
        perror("Errore nella receive");
        running = 0;
    	return -1;
	}

    if (cmd.type != HANDLE_CARD) {
		printf("Ottenuto comando inaspettato (%d) per HANDLE_CARD\n", cmd.type);
		running = 0;
		return -1;
    }

	Card card;
	int ret = cmd_to_card(&cmd, &card);
	
	if(ret < 0) {
		printf("Impossibile deserializzare card\n");
		running = 0;
		return -1;
	}

	// stampa card
	printf("Ottenuta card:\n");
	printf("%s\n", card.testo);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d", card.id, card.utente, card.timestamp.tm_mday, card.timestamp.tm_mon + 1, card.timestamp.tm_year + 1900, card.timestamp.tm_hour, card.timestamp.tm_min, card.timestamp.tm_sec);
	printf("%s\n", buffer);

	return card.id;
}

void *listener_thread(void *arg) {
	while(!registered) {}

    while (running) {
        // effettua il ciclo della card:
        // ottieni card dal server
        int id = get_card();
		
		break;

        // - aspetta
        // int n = rand() % 30;
        // sleep(n);
        // // - ottieni lista client
        // Command cmd = {.type = REQUEST_USER_LIST};
        // if (send_command(&cmd, client_sock, &client_sock_m) < 0) {
        //     perror("Errore nella send");
        //     running = 0;
        //     break;
        // }

        // if (recv_command(&cmd, buffer, client_sock, &client_sock_m) < 0) {
        //     perror("Errore nella receive");
        //     running = 0;
        //     break;
        // }

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

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) continue;
        buffer[strcspn(buffer, "\n")] = '\0';

        Command cmd = {0};
        buf_to_cmd(buffer, &cmd);

		if(cmd.type == HELLO) registered = 1;

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
