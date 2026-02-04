#ifndef INCLUDES_H
#define INCLUDES_H

typedef enum {
  // client -> server
  CREATE_CARD,
  HELLO,
  QUIT,
  PONG_LAVAGNA,
  ACK_CARD,
  REQUEST_USER_LIST,
  CARD_DONE,

  // server -> client
  SEND_USER_LIST,
  PING_USER,
  HANDLE_CARD,
  OK,
  ERR,

  // console -> client
  SHOW_LAVAGNA,
  SHOW_CLIENTS,
  MOVE_CARD,

  // client -> client
  REVIEW_CARD,
  ACK_REVIEW_CARD
} cmd_type;

/*
 * Macro per il numero di tipi di comando
 */
#define NUM_CMD_TYPES (ACK_REVIEW_CARD + 1)

/*
 * Ottiene il tipo di comando a partire dalla stringa che rappresenta la parola
 * chiave del comando effettuando una ricerca sulla mappa comandi
 */
cmd_type str_to_type(const char *keyword);

/*
 * Ottiene la stringa che rappresenta la parola chiave di un comando a partire
 * dal tipo di comando effettuando una ricerca sulla mappa comandi
 */
const char *type_to_str(cmd_type type);


mod_type type_to_mod(cmd_type type);

/*
 * Numero massimo di argomenti per comando
 */
#define MAX_CMD_ARGS 10

/*
 * Numero massimo di caratteri per un comando (incluso \0)
 */
#define CMD_BUF_SIZE 256

/*
 * Un comando è rappresentato da:
 * - il suo tipo
 * - i suoi argomenti, come una lista terminata da NULL
 */
typedef struct {
  cmd_type type;
  const char *args[MAX_CMD_ARGS];
} cmd;

typedef struct {
  cmd_type type;
  const char *str;
} cmd_entry;






#endif