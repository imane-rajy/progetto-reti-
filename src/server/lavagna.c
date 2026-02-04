#include "lavagna.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 30

typedef struct {
	unsigned short id;
} User;

User users[MAX_USERS] = {0};
int num_user = 0;

void inserisci_user(unsigned short id) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].id != 0)
            continue;

        users[i].id = id;
    }

    // errore: spazio esaurito
}

void rimuovi_user(unsigned short id) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].id != id)
            continue;

        users[i].id = 0;
    }
}

int controlla_user(unsigned short id) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].id == id) return 1;
    }

	return 0;
}

#define COL_WIDTH 50

const char *col_names[] = {"TO_DO", "DOING", "DONE"};

Colonna str_to_col(const char *str) {
    for (int i = 0; i < NUM_COLS; i++) {
        if (strcmp(col_names[i], str) == 0) {
            return i;
        }
    }

    return 0;
}

const char *col_to_str(Colonna id) { return col_names[id]; }

Card cards[MAX_CARDS] = {0};
int num_card = 0;

int create_card(int id, Colonna colonna, const char *testo, unsigned short client) {
    // verifica che l'id sia unico
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].id == id) {
            return -1;
        }
    }

    // alloca la card
    int idx = num_card;
    num_card++;
    if (idx >= MAX_CARDS) {
        return -2;
    }

    // imposta i dati della card
    cards[idx].id = id;
    cards[idx].colonna = colonna;

    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';

    cards[idx].utente = client;

	printf("Creata card %d di client %d\n", id, client);
    return idx;
}

int hello(unsigned short client) {
	inserisci_user(client);

	printf("Registrato client %d\n", client);
	return 0;
}

int quit(unsigned short client) {
	rimuovi_user(client);

	printf("Deregistrato client %d\n", client);
	return 0;
}

void gestisci_comando(const Command *cmd, unsigned short port) {
	mostra_lavagna();
	
	if(controlla_user(port) == 0 && cmd->type != HELLO) {
		printf("Ottenuto comando non HELLO da client non registrato %d\n", port);
		return;
	}

    switch (cmd->type) {
		case CREATE_CARD: {
			int id = atoi(cmd->args[0]);
			Colonna colonna = str_to_col(cmd->args[1]);
			const char *testo = cmd->args[2];
			unsigned short client = port;

			create_card(id, colonna, testo, client);
			break;
		}
		case HELLO: {
			hello(port);
			break;
		}
		case QUIT: {
			quit(port);
			break;
		}
		case PONG_LAVAGNA: {
			// pong_lavagna();
			break;
		}
		case ACK_CARD: {
			// ack_card();
			break;
		}
		case REQUEST_USER_LIST: {
			// request_user_list();
			break;
		}
		case CARD_DONE: {
			// card_done();
			break;
		}
		default: break;
	}
}

void mostra_lavagna() {
	system("clear");    

	// stampa header
    for (int c = 0; c < NUM_COLS; c++) {
        printf("%-*s", COL_WIDTH, col_names[c]);
    }
    printf("\n");
    for (int i = 0; i < NUM_COLS * COL_WIDTH; i++) {
		printf("-");
	}
    printf("\n");

    // ottieni il numero di righe
    int max_rows = 0;
    int col_counts[NUM_COLS] = {0};
    for (int i = 0; i < num_card; i++) {
        col_counts[cards[i].colonna]++;
        if (col_counts[cards[i].colonna] > max_rows)
            max_rows = col_counts[cards[i].colonna];
    }

    // realizza una tabella di puntatori a card organizzata per colonne
    Card *col_cards[NUM_COLS][MAX_CARDS] = {0};
    int col_index[NUM_COLS] = {0};
    for (int i = 0; i < num_card; i++) {
        Colonna col = cards[i].colonna;
        col_cards[col][col_index[col]++] = &cards[i];
    }

    // stampa le card
    for (int row = 0; row < max_rows; row++) {
        // 2 righe per card
        for (int l = 0; l < 3; l++) {
            for (int c = 0; c < NUM_COLS; c++) {
                if (row < col_index[c]) {
                    Card *card = col_cards[c][row];
                    char buf[COL_WIDTH + 1];

                    switch (l) {
						// colonna 0: testo
  						case 0:
  						  strncpy(buf, card->testo, COL_WIDTH);
  						  buf[COL_WIDTH] = '\0';
  						  printf("%-*s", COL_WIDTH, buf);
  						  break;

						// colonna 1: altri dati
  						case 1: 
  						  snprintf(buf, sizeof(buf),
  						    "ID:%d Client:%d %02d-%02d-%04d %02d:%02d:%02d",
  						    card->id, card->utente,
  						    card->timestamp.tm_mday, card->timestamp.tm_mon + 1, card->timestamp.tm_year + 1900,
  						    card->timestamp.tm_hour, card->timestamp.tm_min, card->timestamp.tm_sec
  						  );

  						  buf[COL_WIDTH] = '\0';
  						  printf("%-*s", COL_WIDTH, buf);
  						  break;

						// colonna 2: spazio vuoto
						case 2:
							printf("%-*s", COL_WIDTH, ""); // vuoto
					}
                } else {
                    printf("%-*s", COL_WIDTH, ""); // vuoto
                }
            }

            printf("\n");
        }
    }
}

void init_lavagna() { 
	create_card(1, TO_DO, "Implementare integrazione per il pagamento", 0);
    create_card(2, TO_DO, "Implementare sito web servzio", 0);
    create_card(3, TO_DO, "Diagramma delle classi UML", 0);
    create_card(4, TO_DO, "Studio dei requisiti dell'applicazione", 0);
    create_card(5, TO_DO, "Realizzare CRC card", 0);
    create_card(6, TO_DO, "Studio dei casi d'uso", 0);
    create_card(7, TO_DO, "Realizzazione dei flow of events", 0);
    create_card(8, TO_DO, "Diagramma di deployment", 0);
    create_card(9, TO_DO, "Analisi delle classi", 0);
    create_card(10, TO_DO, "Implementare testing del software", 0);

	mostra_lavagna();
}
