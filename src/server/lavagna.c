#include "lavagna.h"

#include <stdio.h>
#include <string.h>

#define COL_WIDTH 30

Card cards[MAX_CARDS] = {0};
int num_card = 0;

int create_card(int id, const char* colonna, const char* testo ){
    for(int i = 0; i < MAX_CARDS; i++){
        if(cards[i].id == id){
            return -1;
        }
    }

    int idx = num_card;

    if(idx >= MAX_CARDS){
        return -2;
    }

    cards[idx].id = id;
    if(strcmp(colonna, "ToDo") == 0){
        cards[idx].colonna = 0;
    }
    else if(strcmp(colonna, "Doing") == 0){
        cards[idx].colonna = 1;
    }
    else if(strcmp(colonna, "Do") == 0){
        cards[idx].colonna = 2;
    }
    else{
        return -3;
    }

    strncpy(cards[idx].testo, testo, MAX_TESTO - 1);
    cards[idx].testo[MAX_TESTO - 1] = '\0';
    cards[idx].utente = -1;

    num_card++;

    return idx;
}

void mostra_lavagna() {
    printf("\n| %-*s | %-*s | %-*s |\n",
           COL_WIDTH, "TO DO",
           COL_WIDTH, "DOING",
           COL_WIDTH, "DONE");

    // linea di separazione
    for (int k = 0; k < 3; k++) {
        printf("|");
        for (int j = 0; j < COL_WIDTH + 2; j++)
            printf("-");
    }
    printf("|\n");

    // separiamo le card per colonna
    Card toDo[MAX_CARDS], doing[MAX_CARDS], done[MAX_CARDS];
    int nToDo = 0, nDoing = 0, nDone = 0;

    for (int i = 0; i < num_card; i++) {
        if (cards[i].colonna == 0) toDo[nToDo++] = cards[i];
        else if (cards[i].colonna == 1) doing[nDoing++] = cards[i];
        else done[nDone++] = cards[i];
    }
    
    int maxRows = nToDo;
    if (nDoing > maxRows) {
        maxRows = nDoing;
    }
    if (nDone > maxRows) {
        maxRows = nDone;

    }

    // stampiamo le card riga per riga
    for (int i = 0; i < maxRows; i++) {
        char *sToDo = (i < nToDo) ? toDo[i].testo : NULL;
        char *sDoing = (i < nDoing) ? doing[i].testo : NULL;
        char *sDone = (i < nDone) ? done[i].testo : NULL;

        int lenToDo = sToDo ? strlen(sToDo) : 0;
        int lenDoing = sDoing ? strlen(sDoing) : 0;
        int lenDone = sDone ? strlen(sDone) : 0;

        // numero di righe necessarie per testi lunghi
        int righe = 1;
        if (lenToDo > COL_WIDTH) righe = (lenToDo + COL_WIDTH - 1)/COL_WIDTH;
        if (lenDoing > COL_WIDTH) { int r = (lenDoing + COL_WIDTH - 1)/COL_WIDTH; if (r > righe) righe = r; }
        if (lenDone > COL_WIDTH) { int r = (lenDone + COL_WIDTH - 1)/COL_WIDTH; if (r > righe) righe = r; }

        for (int r = 0; r < righe; r++) {
            // TO DO
            if (sToDo)
                printf("| %-*.*s ", COL_WIDTH, COL_WIDTH, sToDo + r*COL_WIDTH);
            else
                printf("| %-*s ", COL_WIDTH, "");

            // DOING
            if (sDoing)
                printf("| %-*.*s ", COL_WIDTH, COL_WIDTH, sDoing + r*COL_WIDTH);
            else
                printf("| %-*s ", COL_WIDTH, "");

            // DONE
            if (sDone)
                printf("| %-*.*s |\n", COL_WIDTH, COL_WIDTH, sDone + r*COL_WIDTH);
            else
                printf("| %-*s |\n", COL_WIDTH, "");
        }

        // separatore tra le righe
        printf("|");
        for (int k = 0; k < 3; k++) {
            for (int j = 0; j < COL_WIDTH + 2; j++) printf("-");
            printf("|");
        }
        printf("\n");
    }
}

void gestisci_comando(const Command* cmd, unsigned short port) {
		printf("Ho ottenuto il comando %d con %d argomenti da %d\n", cmd->type, get_argc(cmd), port);
		return;

    switch(cm->type){
        case CREATE_CARD:
<<<<<<< HEAD
            if(cm->args)
            create_card();
=======
            // create_card();
>>>>>>> 82e9106b28960627665a79b29c357f925ca551ea
            break;
        case HELLO:
            // hello();
            break;
        case QUIT:
            // quit();
            break;
        case PONG_LAVAGNA:
            // pong_lavagna();
            break;
        case ACK_CARD:
            // ack_card();
            break;
        case REQUEST_USER_LIST:
            // request_user_list();
            break;
        case CARD_DONE:
            // card_done();
            break;
        default:
            break;
    }
}

void init_lavagna(){
    create_card(1, "ToDo", "Implementare integrazione per il pagamento");
    create_card(2, "ToDo", "Implementare sito web servzio");
    create_card(3, "ToDo", "Diagramma delle classi UML");
    create_card(4, "ToDo", "Studio dei requisiti dell'applicazione");
    create_card(5, "ToDo", "Realizzare CRC card");
    create_card(6, "ToDo", "Studio dei casi d'uso");
    create_card(7, "ToDo", "Realizzazione dei flow of events");
    create_card(8, "Doing", "Diagramma di deployment");
    create_card(9, "ToDo", "Analisi delle classi");
    create_card(10, "ToDo", "Implementare testing del software");
    mostra_lavagna();

}
