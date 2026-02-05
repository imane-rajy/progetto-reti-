#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// numero di caratteri di un singolo evento
#define LOG_SIZE 256

// buffer degli eventi
char logbuf[MAX_LOGS][LOG_SIZE] = {0};

// indice nel buffer degli eventi del prossimo evento da loggare
int logidx = 0;

void log_evento(const char *fmt, ...) {
    // mette gli argomenti forniti nel prossimo evento nel buffer
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logbuf[logidx], LOG_SIZE, fmt, ap); // analoga a printf
    va_end(ap);

    // avanza al prossimo evento
    logidx = (logidx + 1) % MAX_LOGS;
}

void stampa_eventi() {
    // stampa tutti gli eventi nel buffer a partire da quello corrente
    for (int i = 0; i < MAX_LOGS; i++) {
        int idx = (logidx + i) % MAX_LOGS;

        if (logbuf[idx][0]) {
            printf("%s", logbuf[idx]);
        } else {
            // se non c'è un evento, stampa spazio bianco per mantenere il layout
            printf("\n");
        }
    }
}

// carattere con cui si stampa la linea dell'header
#define FILL "="

void stampa_header(const char *s, int width) {
    // controlla la lunghezza di s
    int len = strlen(s);

    // se è maggiore di width, stampa solo s
    if (len >= width) {
        printf("%.*s", width, s);
        return;
    }

    // altrimenti calcola il padding a sinistra e a destra
    int left = (width - len) / 2;
    int right = width - left - len;

    for (int i = 0; i < left; i++)
        printf(FILL);
    printf("%s", s);
    for (int i = 0; i < right; i++)
        printf(FILL);
}
