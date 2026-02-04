#include "lavagna.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

const char *col_to_str(Colonna id) {
	return col_names[id];
}

Card cards[MAX_CARDS] = {0};
int num_card = 0;

int create_card(int id, Colonna colonna, const char* testo, unsigned short client) {
	// verifica che l'id sia unico
  	for(int i = 0; i < MAX_CARDS; i++){
        if(cards[i].id == id){
            return -1;
        }
    }

	// alloca la card
    int idx = num_card;
    num_card++;
    if(idx >= MAX_CARDS){
        return -2;
    }

	// imposta i dati della card
    cards[idx].id = id;
	cards[idx].colonna = colonna;

    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';

	cards[idx].utente = client;

	return idx;
}

void mostra_lavagna() {
	// stampa header
	for (int c = 0; c < NUM_COLS; c++) {
        printf("%-*s", COL_WIDTH, col_names[c]);
    }
    printf("\n");

	// ottieni il numero di righe
    int max_rows = 0;
    int col_counts[NUM_COLS] = {0};
    for (int i = 0; i < num_card; i++) {
        col_counts[cards[i].colonna]++;
        if (col_counts[cards[i].colonna] > max_rows) max_rows = col_counts[cards[i].colonna];
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
		for(int l = 0; l < 2; l++) {
			for (int c = 0; c < NUM_COLS; c++) {
				if (row < col_index[c]) {
					// card da stampare
					Card* card = col_cards[c][row];

					switch(l) {
						case 0:
							printf("%-*s", COL_WIDTH, card->testo);
							break;
						case 1:
							printf("ID: %d | User: %d | Time: %02d-%02d-%04d %02d:%02d:%02d",
								card->id,
								card->utente,
								card->timestamp.tm_mday,
								card->timestamp.tm_mon + 1,
								card->timestamp.tm_year + 1900,
								card->timestamp.tm_hour,
								card->timestamp.tm_min,
								card->timestamp.tm_sec
							);
							break;
					}
				} else {
					printf("%-*s", COL_WIDTH, ""); // vuoto 
				}
			}
        	printf("\n");
        }
    }
}

void gestisci_comando(const Command* cmd, unsigned short port) {
		printf("Ho ottenuto il comando %d con %d argomenti da %d\n", cmd->type, get_argc(cmd), port);
		return;

    switch(cmd->type){
        case CREATE_CARD: {
			int id = atoi(cmd->args[0]);
			Colonna colonna = str_to_col(cmd->args[1]);
			const char* testo = cmd->args[2];
			unsigned short client = port;

            create_card(id, colonna, testo, client);
            break;
		}
        case HELLO: {
            // hello();
            break;
		}
        case QUIT: {
            // quit();
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
        default:
            break;
    }
}

void init_lavagna(){
    create_card(1, TO_DO, "Implementare integrazione per il pagamento", 0);
    create_card(2, TO_DO, "Implementare sito web servzio", 0);
    create_card(3, TO_DO, "Diagramma delle classi UML", 0);
    create_card(4, TO_DO, "Studio dei requisiti dell'applicazione", 0);
    create_card(5, TO_DO, "Realizzare CRC card", 0);
    create_card(6, TO_DO, "Studio dei casi d'uso", 0);
    create_card(7, TO_DO, "Realizzazione dei flow of events", 0);
    create_card(8, TO_DO, "Diagramma di deployment", 0);
    create_card(9, TO_DO, "Analisi delle classi", 0);
    create_card(10, DOING, "Implementare testing del software", 0);
    mostra_lavagna();

}
