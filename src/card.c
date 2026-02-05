#include "card.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void card_to_cmd(const Card *c, Command *cm) {
	static char id[16], col[8], user[16];
  	static char date[32], time[16];
  	static char testo[MAX_TESTO];

  	snprintf(id, sizeof(id), "%d", c->id);
  	snprintf(col, sizeof(col), "%d", c->colonna);
  	snprintf(user, sizeof(user), "%d", c->client);

	snprintf(date, sizeof(date), "%02d-%02d-%04d",
		c->timestamp.tm_mday,
		c->timestamp.tm_mon + 1,
		c->timestamp.tm_year + 1900);

  	snprintf(time, sizeof(time), "%02d:%02d:%02d",
		c->timestamp.tm_hour,
		c->timestamp.tm_min,
		c->timestamp.tm_sec);

  	strncpy(testo, c->testo, MAX_TESTO - 1);
  	testo[MAX_TESTO - 1] = '\0';

  	cm->args[0] = id;
  	cm->args[1] = col;
  	cm->args[2] = user;
  	cm->args[3] = date;
  	cm->args[4] = time;  	
	cm->args[5] = testo;
}

int cmd_to_card(const Command *cm, Card *c) {
	if (get_argc(cm) < 6) return -1;

	int day, mon, year, hour, min, sec;

	c->id = atoi(cm->args[0]);
	c->colonna = (Colonna) atoi(cm->args[1]);
	c->client = atoi(cm->args[2]);

	sscanf(cm->args[3], "%d-%d-%d", &day, &mon, &year);
	sscanf(cm->args[4], "%d:%d:%d", &hour, &min, &sec);

	c->timestamp.tm_mday = day;
	c->timestamp.tm_mon = mon - 1;
	c->timestamp.tm_year = year - 1900;
	c->timestamp.tm_hour = hour;
	c->timestamp.tm_min = min;
	c->timestamp.tm_sec = sec;

	char* pun = c->testo;
	for(int i = 5; i < get_argc(cm); i++) {
		const char* arg = cm->args[i];
		
		strcpy(pun, arg);
		pun += strlen(arg);
		
		*pun++ = ' ';
	}

	return 0;
}
