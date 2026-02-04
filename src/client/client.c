#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

#define CLIENT_ADDR "127.0.0.1"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5678

int client_sock;
volatile int running = 1;

pthread_mutex_t running_mtx = PTHREAD_MUTEX_INITIALIZER;

void* listener_thread(void* arg) {
    char buffer[BUFFER_SIZE];

    while (running) {
        int n = recv(client_sock, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) {
            printf("Connessione chiusa dal server\n");
            running = 0;
            break;
        }

        buffer[n] = '\0';
        printf("\n[SERVER]: %s\n", buffer);
        printf("Inserisci il messaggio da inviare al server: ");
        fflush(stdout);
    }
    return NULL;
}



void* console_thread(void* arg) {
    char buffer[BUFFER_SIZE];

    while (running) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            continue;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("Errore nella send");
            running = 0;
            break;
        }

        if (strcmp(buffer, "QUIT") == 0) {
            running = 0;
            break;
        }
    }
    return NULL;
}


int main(int argc, char* argv[]) {
		// prendi argomenti
    if (argc < 2) {
        printf("Uso: ./client [numero di porta]\n");
        return 1;
    }
  
		// prendi porta 
    unsigned short port = atoi(argv[1]);
    
		// crea socket
    int client_sock;
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket");
        return -1;
    }
		
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
		if(bind(client_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
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
  
		// entra in un ciclo di esecuzione di comandi
    /*
		while(1) {
        printf("Inserisci il messaggio da inviare al server: ");
    
				char buffer[BUFFER_SIZE] = {0};
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        		printf("Errore nella lettura dell'input\n");
            close(client_sock);
            return -1;
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // rimuove il newline

        // invio il comando al server
        if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("Errore nella send");
            close(client_sock);
            return -1;
        }

        printf("Messaggio inviato al server: %s\n", buffer);
		}
    */
    pthread_t t_listener, t_console;
    pthread_mutex_t m
    pthread_create(&t_listener, NULL, listener_thread, NULL);
    pthread_create(&t_console, NULL, console_thread, NULL);

    pthread_join(t_listener, NULL);
    close(client_sock);
    return 0;
}
