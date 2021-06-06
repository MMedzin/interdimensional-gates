#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    while (state != Finish) {
        int perc = random() % 100;

//        if (perc < STATE_CHANGE_PROB) {
//            if (state == InRun) { debug("Zmieniam stan na wysyłanie");
//                changeState(InSend);
//                packet_t *pkt = malloc(sizeof(packet_t));
//                pkt->data = perc;
//                changeTallow(-perc);
//                sleep(SEC_IN_STATE); // to nam zasymuluje, że wiadomość trochę leci w kanale
//                // bez tego algorytm formalnie błędny za każdym razem dawałby poprawny wynik
//                sendPacket(pkt, (rank + 1) % size, TALLOWTRANSPORT);
//                changeState(InRun);debug("Skończyłem wysyłać");
//            } else {
//            }
//        }
//        sleep(SEC_IN_STATE);

        if (perc < STATE_CHANGE_PROB) {
            switch (current_state()) {
                case Rest:
                    sleep(SEC_IN_STATE);
                    broadcast_request_simple(REQ_S);
                    changeState(Wait_Shop);
                    debug("Staję w kolejce do sklepu.");
                    break;
                case Shop:
                    sleep(SEC_IN_STATE);
                    send_ack_s_shopqueue();
                    changeState(Ready);
                    debug("Wychodzę ze sklepu.");
                    break;
                case Ready:
                    sleep(SEC_IN_STATE);
                    broadcast_req_m();
                    changeState(Wait_Medium);
                    debug("Staję w kolejce do medium.");
                    break;
                case Medium:
                    println("Rozsyłam release_m.");
                    sleep(SEC_IN_STATE);
                    send_release_m();
                    changeState(Travel);
                    debug("Jestem w innym wymiarze.");
                    break;
                case Travel:
                    sleep(SEC_IN_STATE);
                    broadcast_req_r();
                    changeState(Wait_Return);
                    debug("Czekam na powrót do domu.");
                    break;
                case Return:
                    sleep(SEC_IN_STATE);
                    send_ack_r_returnqueue();
                    changeState(Rest);
                    debug("Wróciłem do domu.");
                    break;
                default:
                    break;
            }
        }
        sleep(SEC_IN_STATE);
    }
}

