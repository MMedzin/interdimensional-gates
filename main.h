#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "queues.h"
/* odkomentować, jeżeli się chce DEBUGI */
//#define DEBUG
/* boolean */
#define TRUE 1
#define FALSE 0

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

/* stany procesu */
typedef enum {Rest, Wait_Shop, Shop, Ready, Wait_Medium, Medium, Travel, Wait_Return, Return, Finish} state_t;
extern state_t state;
extern int rank;
extern int size;

typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */

    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
//    tunnel_info_t* tunnel_info; // pole przesyłane tylko w komunikatach req_r
    int tunnel; // identyfikator użytego tunelu
    int time; // zegar lamporta w momencie przejścia przez tunel
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;


extern int lamport; // zegar lamporta
int incLamport();
int incMaxLamport(int n);

extern int myShopReqLamport;

extern char* shop_queue;
extern char* return_queue;

extern int ack_s_count; // licznik otrzymanych ACK_S
extern int ack_m_count; // licznik otrzymanych ACK_M
extern int ack_r_count; // licznik otrzymanych ACK_R

extern int used_medium; // indeks medium z jakiego skorzystał proces (istotne tylko w stanach Medium - Return)
extern int travel_time; // stan zegaru lamporta w momencie skorzystania z medium (istotne tylko w stanach Medium - Return)

extern queue_elem* medium_queue_first;

/* typy wiadomości */
#define FINISH 1
#define REQ_S 2
#define REQ_M 3
#define REQ_R 4
#define ACK_S 5
#define ACK_M 6
#define ACK_R 7
#define RELEASE_M 8

/* uzywane wielkości */
#define SHOP_SIZE 2 // pojemność sklepu
#define MEDIUMS_COUNT 3 // ilość dostępnych mediow
#define MAX_MEDIUM_USES 4 // ilość użyć po których medium musi odpocząć
#define MEDIUM_REST_TIME 2 // czas odpoczynku medium

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta

   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
					   "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
                                            UWAGA:
                                                27 == kod ascii escape.
                                                Pierwsze %c[%d;%dm ( np 27[1;10m ) definiuje styl i kolor literek
                                                Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
                                                ...  w definicji makra oznacza, że ma zmienną liczbę parametrów

*/
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d] {%d}: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d] {%d}: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, ##__VA_ARGS__, 27,0,37);

extern int medium_uses_counter[MEDIUMS_COUNT];

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);
void sendPacketNoInc(packet_t *pkt, int destination, int tag);
void changeState( state_t );

state_t current_state();
int get_used_medium();

void broadcast_request(int tag, packet_t *pkt);
void broadcast_request_simple(int tag, int reqLamport);
void send_ack_queue(int tag, char* queue);

void manage_req_s(packet_t *pkt);
void inc_ack_s_count();
int enough_ack_s();
void reset_ack_s_count();
void send_ack_s_shopqueue();

void broadcast_req_m();
void manage_req_m(packet_t *pkt);
void inc_ack_m_count();
int my_medium_free();
int can_enter_medium();
void reset_ack_m_count();
void release_medium(packet_t *pkt);
void save_used_medium();
void send_release_m();

void broadcast_req_r();
void manage_req_r(packet_t *pkt);
void inc_ack_r_count();
int enough_ack_r();
void reset_ack_r_count();
void send_ack_r_returnqueue();

#endif

