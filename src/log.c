#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_SIZE 256

char logbuf[MAX_LOGS][LOG_SIZE];
int logidx = 0;

void log_evento(const char *fmt, ...) {
    // stampa gli argomenti forniti sul prossimo log
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logbuf[logidx], LOG_SIZE, fmt, ap);
    va_end(ap);

    // avanza al prossimo evento
    logidx = (logidx + 1) % MAX_LOGS;
}

void stampa_log() {
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

#define FILL '='

void print_header(const char *s, int width) {
    int len = strlen(s);
    if (len >= width) {
        printf("%.*s", width, s);
        return;
    }

    int left = (width - len) / 2;
    int right = width - left - len;

    for (int i = 0; i < left; i++)
        putchar(FILL);
    fputs(s, stdout);
    for (int i = 0; i < right; i++)
        putchar(FILL);
}
