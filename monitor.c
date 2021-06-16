#include "main.h"
#include "monitor.h"

/* monitor to osobny wątek istniejący tylko u ROOTa, komunikujący się z użytkownikiem */
void *startMonitor(void *ptr) {
    /* Obrazuje pętlę odbierającą komendy od użytkownika */
    char *instring, *token, *saveptr;
    instring = malloc(100);
    int newline;
    char *res;
    while (state != Finish) {
        debug("monitoruję");
        res = fgets(instring, 99, stdin);
        if (res == 0) continue;
        newline = strcspn(instring, "\n");
        if (newline < 2) continue;
        instring[newline] = 0;
        debug("string %s\n", instring);
        token = strtok_r(instring, " ", &saveptr);
        if ((strcmp(token, "exit") == 0) ||
            (strcmp(token, "quit") == 0)) {
            int i;
            for (i = 0; i < size; i++)
                sendPacket(0, i, FINISH);
        } else if (strcmp(token, "send") == 0) {
            token = strtok_r(0, " ", &saveptr);
            int i = 1, data = 0, type;
            if (token) i = atoi(token);
            token = strtok_r(0, " ", &saveptr);
            if (token) {
                if (strcmp(token, "finish") == 0) {
                    type = FINISH;
                    state = Finish;
                    debug("wysyłam finish do %d", i);
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->data = data;
                    sendPacket(pkt, i, type);
                    free(pkt);
                }
            }
        }
    }
    free(instring);
}
