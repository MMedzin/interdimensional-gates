#include "main.h"
#include "watek_komunikacyjny.h"
#include "watek_glowny.h"
#include "monitor.h"
/* wątki */
#include <pthread.h>

/* sem_init sem_destroy sem_post sem_wait */
//#include <semaphore.h>
/* flagi dla open */
//#include <fcntl.h>

state_t state = Rest;
volatile char end = FALSE;
int size, rank, tallow; /* nie trzeba zerować, bo zmienna globalna statyczna */
MPI_Datatype MPI_PAKIET_T;
pthread_t threadKom, threadMon;

int lamport;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tallowMut = PTHREAD_MUTEX_INITIALIZER;

int recordedState;
int involvedInStateRec = 0;
int *channelStates;
int receivedMarkers = 0;

int used_medium = -1;
int travel_time = -1;

char* shop_queue;
char* return_queue;
int medium_uses_counter[MEDIUMS_COUNT];

int ack_s_count = 0;
int ack_m_count = 0;
int ack_r_count = 0;
pthread_mutex_t ack_s_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ack_m_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ack_r_mut = PTHREAD_MUTEX_INITIALIZER;

queue_elem* medium_queue_first = NULL;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
        case MPI_THREAD_SINGLE:
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
            fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
            MPI_Finalize();
            exit(-1);
            break;
        case MPI_THREAD_FUNNELED:
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
            break;
        case MPI_THREAD_SERIALIZED:
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
            break;
        case MPI_THREAD_MULTIPLE:
            printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
            break;
        default:
            printf("Nikt nic nie wie\n");
    }
}

/* srprawdza, czy są wątki, tworzy typ MPI_PAKIET_T
*/
void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems = 5; /* bo packet_t ma trzy pola */
    int blocklengths[5] = {1, 1, 1, 1, 1};
    MPI_Datatype typy[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[5];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);
    offsets[3] = offsetof(packet_t, tunnel);
    offsets[4] = offsetof(packet_t, time);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    shop_queue = malloc(size * sizeof(char));
    return_queue = malloc(size * sizeof(char));
    for(int i = 0; i < size; i++){
        shop_queue[i] = 0;
        return_queue[i] = 0;
    }

    for(int i = 0; i < MEDIUMS_COUNT; i++){
        println("medium_uses_counter[%d] = 0", i);
        medium_uses_counter[i] = 0;
    }

    pthread_create(&threadKom, NULL, startKomWatek, 0);
    if (rank == 0)
    {
        pthread_create(&threadMon, NULL, startMonitor, 0);
    }
    debug("jestem");
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    pthread_mutex_destroy(&stateMut);
    pthread_mutex_destroy(&ack_m_mut);
    pthread_mutex_destroy(&ack_s_mut);
    pthread_mutex_destroy(&ack_r_mut);

    free(shop_queue);
    free(return_queue);
    free_queue(&medium_queue_first);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n");
    pthread_join(threadKom, NULL);
    if (rank == 0)
        pthread_join(threadMon, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

/* opis patrz main.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt = 0;
    if (pkt == 0)
    {
        pkt = malloc(sizeof(packet_t));
        freepkt = 1;
    }
    pkt->src = rank;
    pkt->ts = incLamport();
    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt)
        free(pkt);
}

int incLamport()
{
    pthread_mutex_lock(&lamportMut);
    if (state == Finish)
    {
        pthread_mutex_unlock(&lamportMut);
        return lamport;
    }
    lamport++;
    int tmp = lamport;
    pthread_mutex_unlock(&lamportMut);
    return tmp;
}

int incMaxLamport(int n)
{
    pthread_mutex_lock(&lamportMut);
    if (state == Finish)
    {
        pthread_mutex_unlock(&lamportMut);
        return lamport;
    }
    lamport = (n > lamport) ? n + 1 : lamport + 1;
    int tmp = lamport;
    pthread_mutex_unlock(&lamportMut);
    return tmp;
}

void recordState()
{
    pthread_mutex_lock(&tallowMut);
    recordedState = tallow;
    pthread_mutex_unlock(&tallowMut);
    channelStates = malloc((size) * sizeof(int));
    int i;
    for (i = 0; i < size; i++)
    {
        channelStates[i] = 0;
    }
    involvedInStateRec = 1;
    for (i = 0; i < size; i++)
    {
        if (i != rank)
            sendPacket(0, i, GIVEMESTATE);
    }
    receivedMarkers = 0;
}

void sendState()
{
    int sumState = recordedState;
    int i;
    for (i = 0; i < size; i++)
    {
        sumState += channelStates[i];
    }
    packet_t *pkt = malloc(sizeof(packet_t));
    pkt->data = sumState;
    sendPacket(pkt, 0, STATE);
    involvedInStateRec = 0;
    free(channelStates);
    free(pkt);
}

void changeTallow(int newTallow)
{
    pthread_mutex_lock(&tallowMut);
    if (state == Finish)
    {
        pthread_mutex_unlock(&tallowMut);
        return;
    }
    tallow += newTallow;
    pthread_mutex_unlock(&tallowMut);
}

void changeState(state_t newState)
{
    pthread_mutex_lock(&stateMut);
    if (state == Finish)
    {
        pthread_mutex_unlock(&stateMut);
        return;
    }
    state = newState;
    pthread_mutex_unlock(&stateMut);
}

void broadcast_request(int tag, packet_t *pkt){
    for(int i = 0; i < size; i++){
        if(i != rank){
            sendPacket(pkt, i, tag);
        }
    }
}

void broadcast_request_simple(int tag){
    broadcast_request(tag, 0);
    println("Mam %d ack_s.", ack_s_count);
}

void send_ack_queue(int tag, char* queue){
    for(int i = 0; i < size; i++){
        if (queue[i] != 0){
            sendPacket(0, i, tag);
            queue[i] = 0;
        }
    }
}

state_t current_state(){
    return state;
}

int get_used_medium(){
    return used_medium;
}

void manage_req_s(packet_t *pkt)
{
    if (state == Shop || (state == Wait_Shop && pkt->ts > lamport))
    {
        shop_queue[pkt->src] = 1;
    }
    else
    {
        packet_t *send_pkt = malloc(sizeof(packet_t));
        send_pkt->data = 0;
        sendPacket(send_pkt, pkt->src, ACK_S);
        free(send_pkt);
    }
}

void inc_ack_s_count()
{
    pthread_mutex_lock(&ack_s_mut);
    ack_s_count++;
    pthread_mutex_unlock(&ack_s_mut);
}

int enough_ack_s()
{
    println("Mam %d ack_s, potrzebuję (%d - %d).", ack_s_count, size, SHOP_SIZE);
    return ack_s_count >= (size - SHOP_SIZE);
}

void reset_ack_s_count()
{
    pthread_mutex_lock(&ack_s_mut);
    ack_s_count = 0;
    pthread_mutex_unlock(&ack_s_mut);
}

void send_ack_s_shopqueue(){
    send_ack_queue(ACK_S, shop_queue);
}

void broadcast_req_m(){
    broadcast_request_simple(REQ_M);
    add_to_queue(&medium_queue_first, lamport, rank);
    println("printuje kolejke po b_r_m");
    print_queue(medium_queue_first);
}

void manage_req_m(packet_t *pkt)
{
    add_to_queue(&medium_queue_first, pkt->ts, pkt->src);
    print_queue(medium_queue_first);
    println("first->pending = %d", medium_queue_first->pending);
    packet_t *send_pkt = malloc(sizeof(packet_t));
    send_pkt->data = 0;
    sendPacket(send_pkt, pkt->src, ACK_M);
    free(send_pkt);
}

void inc_ack_m_count()
{
    pthread_mutex_lock(&ack_m_mut);
    ack_m_count++;
    pthread_mutex_unlock(&ack_m_mut);
}

int my_medium_free()
{
    return is_medium_free(medium_queue_first, rank, MEDIUMS_COUNT);
}

int can_enter_medium()
{
    return (ack_m_count >= (size - 1)) && my_medium_free();
}

void save_used_medium()
{
    used_medium = get_position_for_source(medium_queue_first, rank) % MEDIUMS_COUNT;
    travel_time = lamport;
}

void reset_ack_m_count()
{
    pthread_mutex_lock(&ack_m_mut);
    ack_m_count = 0;
    pthread_mutex_unlock(&ack_m_mut);
}

void release_medium(packet_t *pkt)
{
    println("Zwalniam medium: %d.", get_position_for_source(medium_queue_first, pkt->src) % MEDIUMS_COUNT);
    medium_uses_counter[get_position_for_source(medium_queue_first, pkt->src) % MEDIUMS_COUNT]++;
    println("Zwiększyłem licznik użyć.");
    release_for_source(&medium_queue_first, pkt->src);
    println("Zwlniłem medium.");
    check_delete(&medium_queue_first, MEDIUMS_COUNT);
    println("Sprawdziłem usuwanie.");
}

void send_release_m(){
    if(medium_uses_counter[get_position_for_source(medium_queue_first, rank) % MEDIUMS_COUNT] >= MAX_MEDIUM_USES){
        sleep(MEDIUM_REST_TIME); // odpoczynek medium
        medium_uses_counter[get_position_for_source(medium_queue_first, rank) % MEDIUMS_COUNT] = 0;
    }
    broadcast_request_simple(RELEASE_M);
    println("rozesłałem release_m");
    release_for_source(&medium_queue_first, rank);
    println("zwolniłem swoje medium. printuje kolejkę.");
    print_queue(medium_queue_first);
    check_delete(&medium_queue_first, MEDIUMS_COUNT);
    println("sprawdziłem usuwanie. printuje kolejkę jeszcze raz.");
    print_queue(medium_queue_first);
}

void broadcast_req_r(){
    packet_t *send_pkt = malloc(sizeof(packet_t));
    send_pkt->data = 0;
    send_pkt->time = travel_time;
    send_pkt->tunnel = used_medium;
    broadcast_request(REQ_R, send_pkt);
    free(send_pkt);
}

void manage_req_r(packet_t *pkt)
{
    println("Zaczynam manage req_r.");
    println("Tunnel info: tunnel = %d, time = %d.", pkt->tunnel, pkt->time);
    if ((pkt->tunnel == used_medium) && (state == Return ||
                                                    ((state == Travel || state == Wait_Return) &&
                                                     pkt->time >= travel_time)))
    {
        println("if");
        return_queue[pkt->src] = 1;
    }
    else
    {
        println("else");
        packet_t *send_pkt = malloc(sizeof(packet_t));
        send_pkt->data = 0;
        sendPacket(send_pkt, pkt->src, ACK_R);
        free(send_pkt);
    }
    println("Kończę manage req_r.");
}

void inc_ack_r_count(){
    pthread_mutex_lock(&ack_r_mut);
    ack_r_count++;
    pthread_mutex_unlock(&ack_r_mut);
}

int enough_ack_r(){
    return ack_r_count >= (size - 1);
}

void reset_ack_r_count(){
    pthread_mutex_lock(&ack_r_mut);
    ack_r_count = 0;
    pthread_mutex_unlock(&ack_r_mut);
}

void send_ack_r_returnqueue(){
    send_ack_queue(ACK_R, return_queue);
    used_medium = -1;
    travel_time = -1;
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    involvedInStateRec = 0;
    inicjuj(&argc, &argv); // tworzy wątek komunikacyjny w "watek_komunikacyjny.c"
    tallow = 1000;         // by było wiadomo ile jest łoju
    mainLoop();            // w pliku "watek_glowny.c"

    finalizuj();
    return 0;
}

