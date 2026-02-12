#include "../utils/card.h"
#include "../utils/command.h"
#include "../utils/log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// dimensione del buffer di ingresso
#define BUFFER_SIZE 1024

// indirizzo del client
#define CLIENT_ADDR "127.0.0.1"

// indirizzo del server
#define SERVER_ADDR "127.0.0.1"

// porta del server
#define SERVER_PORT 5678

// tempo massimo di attesa prima del CARD_DONE
#define MAX_SLEEP_TIME 5

// numero di porta da cui partono gli utenti
#define MIN_PORT_USERS 5679

// numero massimo di utenti registrati
#define MAX_USERS 30

// socket verso il server (TCP)
int server_sock;

// mutex che protegge il socket verso il server
pthread_mutex_t server_client_m;

// socket verso i server (UDP)
int peer_sock;

// indirizzo di un peer generico
struct sockaddr_in peer_addr;

// identificatore del thread che gestisce le card
pthread_t t_listener;

// flag che rappresenta se il client è registrato
volatile int registered = 0;

// flag che rappresenta se il client è in esecuzione
atomic_int running = 1;

// logica di revisione

// contatore delle revisioni ricevute
int review_ricevuti;

// mutex per proteggere il contatore di revisioni
pthread_mutex_t review_m;

// condition variable per il contatore di revisioni
pthread_cond_t review_cond;

// invia una richiesta di revisione ad un peer
int send_request(unsigned short port) {
    // prepara richiesta di revisione
    Command req = {.type = REVIEW_CARD};

    // specifica la porta del peer
    peer_addr.sin_port = htons(port);

    log_evento("Richiesta review a peer %d\n", port);

    // invia richiesta di revisione
    return sendto_command(&req, peer_sock, &peer_addr);
}

// risponde ad una richiesta di revisione di un peer
int send_review(unsigned short port) {
    // prepara risposta alla revisione
    Command rev = {.type = ACK_REVIEW};

    // specifica la porta del peer
    peer_addr.sin_port = htons(port);

    log_evento("Inviata review a peer %d\n", port);

    // invia risposta alla revisione
    return sendto_command(&rev, peer_sock, &peer_addr);
}

// attende che tutte le review dei peer siano arrivate
int get_review(int num_users) {
    pthread_mutex_lock(&review_m); // blocca il contatore

    // resetta il contatore
    review_ricevuti = 0;

    // bloccati finche non sono arrivate tutte le review
    while (review_ricevuti < num_users) {
        pthread_cond_wait(&review_cond, &review_m);
    }

    pthread_mutex_unlock(&review_m); // sblocca il contatore

    return 0;
}

// logica di gestione pong

// risponde a un PING della lavagna con un PONG
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

// incapsula recv_command per gestire PING che arrivano concorrentemente ai comandi che ci interessano
int recv_con_ping(Command *cm) {
    while (1) {
        // ricevi un comando
        int ret = recv_command(cm, server_sock, NULL, NULL);

        // se c'è un errore o non è PING, restituisci
        if (ret < 0 || cm->type != PING_USER)
            return ret;

        // altrimenti rispondi
        if (pong_lavagna() < 0)
            return -1;
    }
}

// logica di gestione card

// ottiene la lista utenti dal server, non invia la richiesta
int get_user_list(unsigned short users[MAX_USERS - 1], int *num_users) {
    // ottieni lista
    Command cmd = {0};
    if (recv_con_ping(&cmd) < 0) {
        log_evento("Errore nella ricezione card: %s\n", strerror(errno));
        return -1;
    }

    if (cmd.type != SEND_USER_LIST) {
        log_evento("Ottenuto comando inaspettato %s per SEND_USER_LIST\n", cmdtype_to_str(cmd.type));
        return -2;
    }

    log_evento("Ottenuta lista di utenti: ");

    // deserializza lista
    *num_users = 0;
    for (int i = 0; i < get_num_args(&cmd); i++) {
        int cl = atoi(cmd.args[i]);

        if (cl != 0) {
            // riporta client
            users[(*num_users)++] = cl;
            log_evento("%d ", cl);
        }
    }

    log_evento("\n");

    return 0;
}

// ottiene una card dal server, ricevendo anche la lista utenti e facendo ACK_CARD
int get_card(unsigned short users[MAX_USERS - 1], int *num_users) {
    // ottieni card
    Command cmd = {0};
    if (recv_con_ping(&cmd) < 0) {
        log_evento("Errore nella ricezione card: %s\n", strerror(errno));
        return -1;
    }

    if (cmd.type != HANDLE_CARD) {
        log_evento("Ottenuto comando inaspettato %s per HANDLE_CARD\n", cmdtype_to_str(cmd.type));
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
    if (get_user_list(users, num_users) < 0) {
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
    log_evento("ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d\n", card.id, card.port, card.timestamp.tm_mday,
               card.timestamp.tm_mon + 1, card.timestamp.tm_year + 1900, card.timestamp.tm_hour, card.timestamp.tm_min,
               card.timestamp.tm_sec);

    // restituisci solo l'identificatore
    return card.id;
}

// richiede la lista utenti e la legge con get_user_list
int request_user_list(unsigned short users[MAX_USERS - 1], int *num_users) {
    // richiedi la lista utenti
    log_evento("Richiedo la lista di utenti...\n");
    Command req = {.type = REQUEST_USER_LIST};
    send_command(&req, server_sock, &server_client_m);

    // ottieni la lista utenti
    if (get_user_list(users, num_users) < 0) {
        log_evento("Impossibile ottenere lista utenti\n");
        return -1;
    }

    return 0;
}

// invia il CARD_DONE relativo ad una card
void do_card(int card_id) {
    log_evento("Invio il CARD_DONE al server...\n");

    // prepara id
    char id_str[6];
    snprintf(id_str, 6, "%d", card_id);

    // prepara ed invia comando done
    Command ack = {.type = CARD_DONE, .args = {id_str}};
    send_command(&ack, server_sock, &server_client_m);
}

// gestione thread

// larghezza dell'interfaccia del client
#define COL_WIDTH 60

// stampa l'interfaccia del client, cioè gli eventi e una shell
void stampa_interfaccia() {
    // ripulisci lo schermo
    system("clear");

    // stampa gli eventi
    stampa_header("LOG", COL_WIDTH);
    printf("\n");
    stampa_eventi();

    // stampa la shell
    stampa_header("SHELL", COL_WIDTH);
    printf("\n");
    printf("$ ");
    fflush(stdout); // forza a stampare
}

// thread di ascolto, gestisce il ciclo delle card
void *listener_thread(void *arg __attribute__((unused))) {
    while (atomic_load(&running)) {
        // array per gli indici degli utenti
        unsigned short users[MAX_USERS - 1];

        // numero di utenti ottenuti
        int num_users = 0;

        // ottieni card dal server
        int id = get_card(users, &num_users);
        if (id < 0) {
            atomic_store(&running, 0);
            break;
        }

        do {
            // aspetta
            int n = rand() % MAX_SLEEP_TIME;
            sleep(n);

            // ottieni lista utente
            int ret = request_user_list(users, &num_users);
            if (ret < 0) {
                atomic_store(&running, 0);
                break;
            }

            log_evento("Ottenuti %d peer\n", num_users);
        } while (num_users < 1);

        // richiedi review ad ogni client
        for (int i = 0; i < num_users; i++) {
            int port = users[i];
            log_evento("Richiedo la review dell'utente %d...\n", port);
            send_request(port);
        }

        // aspetta le review
        get_review(num_users);

        // invia il card done
        do_card(id);

        // aggiorna interfaccia
        stampa_interfaccia();
    }

    return NULL;
}

// thread console, gestisce i comandi da tastiera
void *console_thread(void *arg __attribute__((unused))) {
    while (atomic_load(&running)) {
        // aggiorna interfaccia
        stampa_interfaccia();

        // ottieni comando
        char buffer[BUFFER_SIZE];
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }
        buffer[strcspn(buffer, "\n")] = '\0';

        // se è comando vuoto non inviare
        if (*buffer == '\0') {
            continue;
        }

        // interpreta comando
        Command cmd = {0};
        buf_to_command(buffer, &cmd);

        // se di HELLO, assumi che sia registrato
        if (cmd.type == HELLO) {
            if (registered == 0) {
                pthread_create(&t_listener, NULL, listener_thread, NULL);
            }

            registered = 1;
        }

        // se di QUIT, assumi che sia deregistrato
        if (cmd.type == QUIT) {
            if (registered == 1) {
                pthread_cancel(t_listener);
            }

            registered = 0;
        }

        // invia il comando
        if (send_command(&cmd, server_sock, &server_client_m) < 0) {
            perror("Errore nella send");
            atomic_store(&running, 0);
            break;
        }
    }

    return NULL;
}

// thread review, gestisce le review fra peer
void *review_thread(void *arg __attribute__((unused))) {
    while (atomic_load(&running)) {
        // comando che conterrà le richieste
        Command cmd = {0};

        // conterrà le porte dei peer
        unsigned short port;

        // ricevi un comando da un peer
        int ret = recvfrom_command(&cmd, peer_sock, &port);
        if (ret < 0) {
            log_evento("Errore nella recv da peer\n");
        }

        // discrimina sulla base del comando
        switch (cmd.type) {
            case REVIEW_CARD:
                // ci è stata chiesta una review, rispondi
                send_review(port);
                break;
            case ACK_REVIEW: {
                pthread_mutex_lock(&review_m); // blocca il contatore

                // questo è di interesse a get_review, segnala
                review_ricevuti++;
                pthread_cond_signal(&review_cond);

                log_evento("Ottenuta review da peer %d\n", port);
                pthread_mutex_unlock(&review_m); // sblocca il contatore
                break;
            }
            default:
                log_evento("Comando inaspettato %s da peer %d\n", cmdtype_to_str(cmd.type), port);
                continue;
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

    if (port < 5679 || port >= 5679 + MAX_USERS) {
        fprintf(stderr, "Errore: la porta deve essere compresa tra %d e %d\n", 5679, 5679 + MAX_USERS);
        return 1;
    }

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
    pthread_t t_console, t_review;

    pthread_create(&t_console, NULL, console_thread, NULL);
    pthread_create(&t_review, NULL, review_thread, NULL);

    pthread_join(t_console, NULL);
    pthread_join(t_review, NULL);

    close(server_sock);

    return 0;
}
