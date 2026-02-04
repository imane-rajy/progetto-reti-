#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 5678
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 256

int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if (argc < 2) {
        printf("Uso: ./client [numero di porta]\n");
        return 1;
    }

  
    // configura socket
    port = atoi(argv[1]);
    if (configure_net() < 0) {
        return 1;
    }

    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Errore nella creazione del socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Indirizzo non valido\n");
        return -1;
    }

   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connessione fallita\n");
        return -1;
    }
    
    while(1){
        printf("Inserisci il messaggio da inviare al server: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Errore nella lettura dell'input\n");
            close(sock);
            return -1;
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // rimuove il newline

        // Invio del messaggio al server
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
