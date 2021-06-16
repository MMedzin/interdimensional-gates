#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    while (state != Finish) {
        long perc = random() % 100;

        if (perc < STATE_CHANGE_PROB) {
            switch (current_state()) {
                case Rest:
                    sleep(SEC_IN_STATE);
                    myShopReqLamport = incLamport();
                    changeState(Wait_Shop);
                    broadcast_request_simple(REQ_S, myShopReqLamport);
                    println("Staję w kolejce do sklepu.");
                    break;
                case Shop:
                    sleep(SEC_IN_STATE);
                    changeState(Ready);
                    send_ack_s_shopqueue();
                    println("Wychodzę ze sklepu.");
                    break;
                case Ready:
                    sleep(SEC_IN_STATE);
                    changeState(Wait_Medium);
                    broadcast_req_m();
                    println("Staję w kolejce do medium.");
                    break;
                case Medium:
                    sleep(SEC_IN_STATE);
                    changeState(Travel);
                    send_release_m();
                    println("Jestem w innym wymiarze.");
                    break;
                case Travel:
                    sleep(SEC_IN_STATE);
                    changeState(Wait_Return);
                    broadcast_req_r();
                    println("Czekam na powrót do domu.");
                    break;
                case Return:
                    sleep(SEC_IN_STATE);
                    changeState(Rest);
                    send_ack_r_returnqueue();
                    println("Wróciłem do domu.");
                    break;
                default:
                    break;
            }
        }
        sleep(SEC_IN_STATE);
    }
}

