#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    int isRecordingState = 0;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( state!=Finish ) {
        debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        incMaxLamport(pakiet.ts);
        switch ( status.MPI_TAG ) {
            case FINISH:
                changeState(Finish);
                break;
            case REQ_S:
                manage_req_s(&pakiet);
                break;
            case ACK_S:
                if (current_state() == Wait_Shop){
                    inc_ack_s_count();
                    if (enough_ack_s()){
                        changeState(Shop);
                        reset_ack_s_count();
                        println("Wchodzę do sklepu.");
                    }
                }
                break;
            case REQ_M:
                manage_req_m(&pakiet);
                break;
            case ACK_M:
                if (current_state() == Wait_Medium){
                    inc_ack_m_count();
                    if (can_enter_medium()){
                        save_used_medium();
                        changeState(Medium);
                        reset_ack_m_count();
                        println("Wchodzę do tunelu. Używane medium: %d.", get_used_medium());
                    }
                }
                break;
            case RELEASE_M:
                release_medium(&pakiet);
                if (current_state() == Wait_Medium && can_enter_medium()){
                    save_used_medium();
                    changeState(Medium);
                    reset_ack_m_count();
                    println("Wchodzę do tunelu. Używane medium: %d.", get_used_medium());
                }
                break;
            case REQ_R:
                manage_req_r(&pakiet);
                break;
            case ACK_R:
                if(current_state() == Wait_Return){
                    inc_ack_r_count();
                    if(enough_ack_r()){
                        changeState(Return);
                        reset_ack_r_count();
                        println("Wracam do domu.");
                    }
                }
                break;
        }
    }
}

