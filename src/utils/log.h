#ifndef LOG_H
#define LOG_H

// numero di eventi nel buffer di eventi
#define MAX_LOGS 10

// mette un evento nel buffer di eventi (non lo stampa). va usata come la printf
void log_evento(const char *fmt, ...);

// stampa il buffer di eventi
void stampa_eventi();

// stampa un header formattato come "====stringa===="
void stampa_header(const char *s, int width);

#endif
