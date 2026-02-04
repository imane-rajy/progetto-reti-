#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

#define CLIENT_ADDR "127.0.0.1"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5678

int main(int argc, char* argv[]) {
		// prendi argomenti
    if (argc < 2) {
        printf("Uso: ./client [numero di porta]\n");
        return 1;
    }
  
		// prendi porta 
    unsigned short port = atoi(argv[1]);
    
		// crea socket
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket");
        return -1;
    }

		// configura indirizzo client	
  	struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
  
    if (inet_pton(AF_INET, CLIENT_ADDR, &client_addr.sin_addr) <= 0) {
        printf("Indirizzo client non valido\n");
        return -1;
    }

		// collega ad indirizzo client 
		if(bind(sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
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
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        return -1;
    }
   
		// entra in un ciclo di esecuzione di comandi
    char buffer[BUFFER_SIZE] = {0};
    while(1){
        printf("Inserisci il messaggio da inviare al server: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        		printf("Errore nella lettura dell'input\n");
            close(sock);
            return -1;
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // rimuove il newline

        // invio il comando al server
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Errore nell'invio");
            close(sock);
            return -1;
        }

        printf("Messaggio inviato al server: %s\n", buffer);
		}

    close(sock);
    return 0;
}
