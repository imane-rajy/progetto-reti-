#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include "lavagna.h"
#define PORT 5678
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024



int main() {
    int server_fd, new_socket, max_sd, sd, activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address), client_socket[MAX_CLIENTS] = {0};
    char buffer[BUFFER_SIZE] = {0};
    fd_set readfds;
    init_lavagna();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallita");
        exit(EXIT_FAILURE);
    }

   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallito");
        exit(EXIT_FAILURE);
    }


    if (listen(server_fd, 10) < 0) {
        perror("Listen fallito");
        exit(EXIT_FAILURE);
    }

    printf("Server TCP in ascolto sulla porta %d...\n", PORT);

    while (1) {
       
        FD_ZERO(&readfds);

        
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

       
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }


        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Select fallito");
        }


        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept fallito");
                exit(EXIT_FAILURE);
            }

            printf("Nuova connessione stabilita con client (socket fd: %d)\n", new_socket);


            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Aggiunto nuovo client nella posizione %d\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {

                    printf("Client disconnesso (socket fd: %d)\n", sd);
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    gestisci_comando(buffer, sd);
                }
            }
        }
    }

    return 0;
}
