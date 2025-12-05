#include <unistd.h>
#include <stdbool.h>
#include "list.h"
#include "display_messages.h"
#include <ncurses.h>
#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "remote-char.h"
#include <pthread.h>

void *  context;

#define NUM_THREADS 5 // 4 -> lizards, 5th -> bots
#define LIZARDS_LIMIT 26
#define BOT_LIMIT (WINDOW_SIZE*WINDOW_SIZE/3)
#define COCKROACH_LIMIT (BOT_LIMIT/2)
#define WASP_LIMIT (BOT_LIMIT/2)
// Mutex declaration and initialization
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to pass data to the thread
typedef struct {

    // int thread_id[NUM_THREADS];
    int unique_id;
    char **tabuleiro_lizards;
    lizard_t *lizard_head, *terminated_lizard_head;
    cockroach_t *cockroaches;
    wasp_t *wasps;
    lizard_t **vector_heads;
    char alfabeto[LIZARDS_LIMIT];
    int num_id_atribuidos;
    int letter_pending;
    WINDOW * my_win;
    void *bot_responder;
    void *sock_recv;
    void *vector_responder[4];
    int sync;
    //sockets!
    void *publisher;
} ThreadData;

char** create_tabuleiro(){

    char **tabuleiro = (char**) malloc(WINDOW_SIZE*sizeof(char*));

    if (tabuleiro == NULL) {
        // Handle the memory allocation error, perhaps by returning NULL
        return NULL;
    }

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        tabuleiro[i] = (char*) malloc(WINDOW_SIZE * sizeof(char));

        if (tabuleiro[i] == NULL) {
            // If allocation fails, free previously allocated memory and return NULL
            for (int j = 0; j < i; j++) {
                free(tabuleiro[j]);
            }
            free(tabuleiro);
            return NULL;
        }
    }

    return tabuleiro;
}

//inicializa o tabuleiro de lizards
void init_tabuleiro(char ** tabuleiro){


    for(int i=0; i<WINDOW_SIZE; i++){

        for (int j = 0; j < WINDOW_SIZE; j++){

            tabuleiro[i][j] = ' ';
        }
    }
}

//da free do tabuleiro
void free_tabuleiro(char **tabuleiro){

    for(int i=0; i<WINDOW_SIZE; i++){
        free(tabuleiro[i]);
    }
    
    free(tabuleiro);
}

//inicia o alfabeto de a-z;
void init_alfabeto(char *alfabeto, char lizard_id){

    //alfabeto
    for(int j=0; j<26; j++){

        alfabeto[j] = lizard_id;
        lizard_id++;
    }
}

//calculates a new position
void new_position(int* x, int *y, remote_char_t m){

    switch (m.direction)
    {
    case UP:
        (*x) --;
        if(*x ==0)
            *x = 2;
        break;
    case DOWN:
        (*x) ++;
        if(*x ==WINDOW_SIZE-1)
            *x = WINDOW_SIZE-3;
        break;
    case LEFT:
        (*y) --;
        if(*y ==0)
            *y = 2;
        break;
    case RIGHT:
        (*y) ++;
        if(*y ==WINDOW_SIZE-1)
            *y = WINDOW_SIZE-3;
        break;
    default:
        break;
    }

}

//checks if there is a lizard in a certain position of the tabuleiro
char is_there_lizard(char **tabuleiro_lizards, int x, int y){

    if(x <WINDOW_SIZE && x >=0 && y < WINDOW_SIZE && y >=0){

        return tabuleiro_lizards[x][y];
    }

    return ' ';
}

int search_terminated_cockroach(int *terminated_cockroaches){

    int aux;
     
    for (int i = 0; i < COCKROACH_LIMIT; i++)
    {
        if(terminated_cockroaches[i] != -1){

            aux = terminated_cockroaches[i];
            terminated_cockroaches[i] = -1;
            return aux;
        }
    }
    
    return 0;
}

int search_terminated_wasps(int *terminated_wasps){

    int aux;
     
    for (int i = 0; i < WASP_LIMIT; i++)
    {
        if(terminated_wasps[i] != -1){

            aux = terminated_wasps[i];
            terminated_wasps[i] = -1;
            return aux;
        }
    }
    
    return 0;
}

//returns the id of a cockroach if it is in position (x,y); returns -1 if there isn't any cockroach
int is_there_cockroach(cockroach_t *array, int x, int y){

    for (int i = 0; i < COCKROACH_LIMIT; i++)
    {
        if(array[i].pos_x==x && array[i].pos_y==y){
            return i+1;
        }
    }
    return -1;
    
}

int is_there_wasp(wasp_t *array, int x, int y)
{
    for (int i = 0; i < WASP_LIMIT; i++)
    {
        if(array[i].pos_x==x && array[i].pos_y==y){
            return i+1;
        }
    }
    return -1;
}

//finds an init position for the lizard
int* random_position(char **matrix, int *pos_array){

    do
    {
        pos_array[0] = rand()%WINDOW_SIZE;
        pos_array[1] = rand()%WINDOW_SIZE;
    } while(matrix[pos_array[0]][pos_array[1]] != ' ');

    return pos_array;
}

void connection_lizard(ThreadData *thread_data, remote_char_t message, int *prev_x ,int *prev_y,int *pos_array, void *responder, int *pos_x, int *pos_y){

    int error;
    lizard_t *lizard_aux;
    int score = 0;

    pthread_mutex_lock(&myMutex);
    //if no client terminates!
    if( thread_data->terminated_lizard_head == NULL){
        message.lizard.id=thread_data->alfabeto[thread_data->letter_pending];
        if(thread_data->letter_pending < LIZARDS_LIMIT){
            thread_data->letter_pending+=1;
        }
    }
    else{
        message.lizard.id=thread_data->terminated_lizard_head->id;
        thread_data->terminated_lizard_head = remove_lizard(thread_data->terminated_lizard_head, thread_data->terminated_lizard_head->id);
    }
    pthread_mutex_unlock(&myMutex);

    pthread_mutex_lock(&myMutex);
    pos_array = random_position(thread_data->tabuleiro_lizards, pos_array);
    pthread_mutex_unlock(&myMutex);

    *pos_x = pos_array[0];
    *pos_y = pos_array[1];
    *prev_x = *pos_x;
    *prev_y = *pos_y;

    pthread_mutex_lock(&myMutex);
    thread_data->tabuleiro_lizards[*pos_x][*pos_y] = message.lizard.id;
    pthread_mutex_unlock(&myMutex);

    message.lizard.pos_x = *pos_x;
    message.lizard.pos_y = *pos_y;

    error = zmq_send(responder,&message,sizeof(message),0);
    if(error < 0){
        perror("Error sending message of connection lizard!\n");
        exit(EXIT_FAILURE);
    }

    //add to lizard List!!
    pthread_mutex_lock(&myMutex);
    thread_data->lizard_head = criaNovoNoLista(thread_data->lizard_head, message.lizard.id, *pos_x, *pos_y, score);
    pthread_mutex_unlock(&myMutex);

    lizard_aux=search_lizard(thread_data->lizard_head,message.lizard.id);
    lizard_aux->orientation = message.lizard.orientation;

    
    pthread_mutex_lock(&myMutex);
    thread_data->num_id_atribuidos+=1;
    pthread_mutex_unlock(&myMutex);
}

void roaches_connection(ThreadData *thread_data, int *terminated_cockroaches, int *num_id_roaches, int *roach_id, remote_char_t message, bool *authorization, int *pos_x, int *pos_y, int *prev_x, int *prev_y, int *how_many_terminated){

    int id;
    int error;
    //enviar o id ao roach
    if (*num_id_roaches<=COCKROACH_LIMIT)
    {   
        if(*how_many_terminated > 0){
            
            id = search_terminated_cockroach(terminated_cockroaches);
            message.cockroach.id=id;
            thread_data->cockroaches[(message.cockroach.id-1)].id = message.cockroach.id;      // stores the cockroach with id x in the slot (x-1) of the cockroach array (this way we can search the cockroach by its id directly in the structure)
            thread_data->cockroaches[(message.cockroach.id-1)].score = message.cockroach.score;
            *how_many_terminated-=1;
        }
        else{
            message.cockroach.id=*roach_id;
            thread_data->cockroaches[(message.cockroach.id-1)].id = message.cockroach.id;      // stores the cockroach with id x in the slot (x-1) of the cockroach array (this way we can search the cockroach by its id directly in the structure)
            thread_data->cockroaches[(message.cockroach.id-1)].score = message.cockroach.score;
            *roach_id+=1;
        }
        *num_id_roaches+=1;

        //atribuir posição no jogo
        while(!(*authorization)){
            int x_candidate = rand()%WINDOW_SIZE;
            int y_candidate = rand()%WINDOW_SIZE;
            if (is_there_lizard(thread_data->tabuleiro_lizards,x_candidate,y_candidate)==' ' && is_there_wasp(thread_data->wasps, x_candidate,y_candidate)==-1)
            {
                *pos_x=x_candidate;
                *pos_y=y_candidate;
                *prev_x=*pos_x;
                *prev_y=*pos_y;
                *authorization=TRUE;
                message.cockroach.pos_x=*pos_x;
                message.cockroach.pos_y=*pos_y;
                thread_data->cockroaches[(message.cockroach.id-1)].pos_x=*pos_x;
                thread_data->cockroaches[(message.cockroach.id-1)].pos_y=*pos_y;
            }
        }
        error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
        if(error < 0){
            perror("Error sending message of connection roach");
            exit(EXIT_FAILURE);
        }
    }else{
        message.terminate=TRUE;
        error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
    }
}

void clear_previous_lizard_body(ThreadData *thread_data, display_message display, void *publisher, direction_t prev_orientation, int prev_x, int prev_y){

    char dummy;
    int error;
    lizard_t *lizard_aux;

    display.informative=FALSE;
    display.x=prev_x;
    display.y=prev_y;
    display.symbol=' ';
    pthread_mutex_lock(&myMutex);
    error=zmq_send(publisher,&display, sizeof(display),0);
    if (error<0)
    {
        perror("Something went wrong");
        exit(EXIT_FAILURE);
    }
    
    //protect shared memory
    wmove(thread_data->my_win,prev_x,prev_y);
    waddch(thread_data->my_win,' '|A_BOLD);
    thread_data->tabuleiro_lizards[prev_x][prev_y]=' ';
    pthread_mutex_unlock(&myMutex);
    switch (prev_orientation)
    {
    case LEFT:
        for (int i = 1; i <=5; i++)
        {
            if ((dummy=is_there_lizard(thread_data->tabuleiro_lizards,prev_x,(prev_y+i))) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if (lizard_aux->orientation==LEFT)        //temos que saber que lizard é que é, para saber qual a sua orientação. Se for a mesma do caso não mexemos mais!
                {
                    break;
                }
                else
                {
                    continue;
                }   
            }
            else if (is_there_cockroach(thread_data->cockroaches,prev_x,(prev_y+i))!=-1)
            {
                continue;
            }
            
            else
            {
                if ((prev_y+i)>=0 && (prev_y+i)<WINDOW_SIZE)
                {
                    display.informative=FALSE;
                    display.x=prev_x;
                    display.y=prev_y+i;
                    display.symbol=' ';
                    //send
                    pthread_mutex_lock(&myMutex);
                    error = zmq_send(publisher,&display,sizeof(display),0);
                    if(error < 0){
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    } 

                    wmove(thread_data->my_win,prev_x,(prev_y+i));
                    waddch(thread_data->my_win,' '| A_BOLD);
                    wrefresh(thread_data->my_win);
                    pthread_mutex_unlock(&myMutex);
                        
                }
                
            }
            
            
        }
    case RIGHT:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,prev_x,(prev_y-i))) != ' ' )
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if (lizard_aux->orientation == RIGHT)        //temos que saber que lizard é que é, para saber qual a sua orientação. Se for a mesma do caso não mexemos mais!
                {
                    break;
                }
                else
                {
                    continue;
                }   
            }
            else if (is_there_cockroach(thread_data->cockroaches,prev_x,(prev_y-i))!=-1)
            {
                continue;
            }
            
            else if((prev_y-i)>=0 && (prev_y-i)<WINDOW_SIZE)
            {
                display.informative=FALSE;
                display.x=prev_x;
                display.y=prev_y-i;
                display.symbol=' ';
                pthread_mutex_lock(&myMutex);
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }

                wmove(thread_data->my_win,prev_x,(prev_y-i));
                waddch(thread_data->my_win,' '| A_BOLD);
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
            }
            
            
        }
    case UP:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,(prev_x+i),prev_y)) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if (lizard_aux->orientation==UP)        //temos que saber que lizard é que é, para saber qual a sua orientação. Se for a mesma do caso não mexemos mais!
                {
                    break;
                }
                else
                {
                    continue;
                }   
            }
            else if (is_there_cockroach(thread_data->cockroaches,(prev_x+i),prev_y)!=-1)
            {
                /* code */
            }
            
            else if((prev_x+i)>=0 && (prev_x+i)<WINDOW_SIZE )
            {
                display.informative=FALSE;
                display.x=prev_x+i;
                display.y=prev_y;
                display.symbol=' ';
                pthread_mutex_lock(&myMutex);
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wmove(thread_data->my_win,(prev_x+i),prev_y);
                waddch(thread_data->my_win,' '| A_BOLD);
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
            }
            
            
        }
    case DOWN:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,(prev_x-i),prev_y)) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if (lizard_aux->orientation==DOWN)        //temos que saber que lizard é que é, para saber qual a sua orientação. Se for a mesma do caso não mexemos mais!
                {
                    break;
                }
                else
                {
                    continue;
                }   
            }
            else if (is_there_cockroach(thread_data->cockroaches,(prev_x-i),prev_y)!=-1)
            {
                continue;
            }
            
            else if((prev_x-i)>=0 && (prev_x-i)<WINDOW_SIZE)
            {
                display.informative=FALSE;
                display.x=prev_x-i;
                display.y=prev_y;
                display.symbol=' ';
                pthread_mutex_lock(&myMutex);
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wmove(thread_data->my_win,(prev_x-i),prev_y);
                waddch(thread_data->my_win,' '| A_BOLD);
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
            }
            
            
        }
        
        break;
    
    default:
        break;
    }
}

//ajustar para quando tiver um lizard vencedor em campo! isso ainda nao ta feito####
void terminate_lizard(display_message display, ThreadData *thread_data, remote_char_t message, void *publisher, direction_t prev_orientation,  void *responder){

    int error;
    lizard_t *aux_terminated_lizard = NULL;

    aux_terminated_lizard = search_lizard(thread_data->lizard_head, message.lizard.id);

    //search lizard
    if (0 >= aux_terminated_lizard->score && aux_terminated_lizard->score < 50)
    {
        //tabuleiro update!
        pthread_mutex_lock(&myMutex);
        thread_data->tabuleiro_lizards[message.lizard.pos_x][message.lizard.pos_y] = ' ';
        thread_data->terminated_lizard_head = criaNovoNoLista(thread_data->terminated_lizard_head, aux_terminated_lizard->id, aux_terminated_lizard->pos_x, aux_terminated_lizard->pos_y, aux_terminated_lizard->score);
        pthread_mutex_unlock(&myMutex);
        clear_previous_lizard_body(thread_data, display, publisher, aux_terminated_lizard->orientation, aux_terminated_lizard->pos_x, aux_terminated_lizard->pos_y);
    }

    //remove the lizard from the lizard list!!
    pthread_mutex_lock(&myMutex);
    thread_data->lizard_head = remove_lizard(thread_data->lizard_head, message.lizard.id);
    thread_data->num_id_atribuidos--;
    pthread_mutex_unlock(&myMutex);

    //send message to client to terminate!
    error = zmq_send(responder,&message,sizeof(message),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

}

void terminate_roach(ThreadData *thread_data, int *terminated_roaches, remote_char_t *message, bool *authorization, int *how_many_terminated ){
    
    display_message display;
    char board_rid[5]="12345";

    for (int i = 0; i < COCKROACH_LIMIT; i++)
    {
        if (terminated_roaches[i]==-1)
        {
            terminated_roaches[i]=message->cockroach.id;
            break;
        }
    }
    
    
    int error;
    int underlying=is_there_cockroach(thread_data->cockroaches,thread_data->cockroaches[(message->cockroach.id-1)].pos_x,thread_data->cockroaches[(message->cockroach.id-1)].pos_y);


    if(underlying == -1 || underlying==message->cockroach.id){      //checks if there was a cockroach beneath


        pthread_mutex_lock(&myMutex);
        wmove(thread_data->my_win,thread_data->cockroaches[(message->cockroach.id-1)].pos_x,thread_data->cockroaches[(message->cockroach.id-1)].pos_y);
        waddch(thread_data->my_win,' ');
        pthread_mutex_unlock(&myMutex);
        display.x=thread_data->cockroaches[(message->cockroach.id-1)].pos_x;
        display.y=thread_data->cockroaches[(message->cockroach.id-1)].pos_y;
        display.symbol=' ';
        display.informative = FALSE;
        pthread_mutex_lock(&myMutex);
        error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&myMutex);
    }
    else
    {
        pthread_mutex_lock(&myMutex);
        wmove(thread_data->my_win,thread_data->cockroaches[(message->cockroach.id-1)].pos_x,thread_data->cockroaches[(message->cockroach.id-1)].pos_y);
        waddch(thread_data->my_win,board_rid[(thread_data->cockroaches[(underlying-1)].score-1)]|A_BOLD);
        pthread_mutex_unlock(&myMutex);
        display.x=thread_data->cockroaches[(message->cockroach.id-1)].pos_x;
        display.y=thread_data->cockroaches[(message->cockroach.id-1)].pos_y;
        display.symbol=board_rid[(thread_data->cockroaches[(underlying-1)].score-1)];
        display.informative = FALSE;

        pthread_mutex_lock(&myMutex);
        // maybe aqui mando a mensagem para as outras threads para mostrarem no client Lizard!!
        error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&myMutex);
    }

    (*how_many_terminated)+=1;
    thread_data->cockroaches[(message->cockroach.id-1)].id = -1;
    wrefresh(thread_data->my_win);
        
}

void terminate_wasp(ThreadData *thread_data, int *terminated_wasps, remote_char_t *message, bool *authorization, int *how_many_terminated){
    display_message display;
    int error;
    
    for (int i = 0; i < WASP_LIMIT; i++)
    {
        if (terminated_wasps[i]==-1)
        {
            terminated_wasps[i]=message->wasp.id;
            break;
        }
    }
    
    pthread_mutex_lock(&myMutex);
    wmove(thread_data->my_win,thread_data->wasps[(message->wasp.id-1)].pos_x,thread_data->wasps[(message->wasp.id-1)].pos_y);
    waddch(thread_data->my_win,' ');
    pthread_mutex_unlock(&myMutex);
    display.x=thread_data->wasps[(message->wasp.id-1)].pos_x;
    display.y=thread_data->wasps[(message->wasp.id-1)].pos_y;
    display.symbol=' ';
    display.informative=FALSE;
    pthread_mutex_lock(&myMutex);
    error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&myMutex);

    *how_many_terminated+=1;
    thread_data->wasps[(message->wasp.id-1)].id = -1;
    wrefresh(thread_data->my_win);
    
}

void wasps_connection(ThreadData *thread_data, int *terminated_wasps, int *num_id_wasps, int *wasp_id, remote_char_t message, bool *authorization, int *pos_x,int *pos_y, int *prev_x,int *prev_y, int *how_many_terminated_wasps){
    
    int id;
    int error;

    if(*num_id_wasps<=WASP_LIMIT){

        if(*how_many_terminated_wasps > 0){
            
            id = search_terminated_wasps(terminated_wasps);
            message.wasp.id=id;
            thread_data->wasps[(message.wasp.id-1)].id=message.wasp.id;
            *how_many_terminated_wasps-=1;
        }
        else{
            message.wasp.id=*wasp_id;
            thread_data->wasps[(message.wasp.id-1)].id = message.wasp.id;      // stores the wasp with id x in the slot (x-1) of the wasp array (this way we can search the wasp by its id directly in the structure)
            *wasp_id+=1;
        }
        *num_id_wasps+=1;

        while(!(*authorization)){
            int x_candidate = rand()%WINDOW_SIZE;
            int y_candidate = rand()%WINDOW_SIZE;
            if ((is_there_lizard(thread_data->tabuleiro_lizards,x_candidate,y_candidate)==' ') && is_there_cockroach(thread_data->cockroaches,x_candidate,y_candidate)==-1)
            {
                *pos_x=x_candidate;
                *pos_y=y_candidate;
                *prev_x=*pos_x;
                *prev_y=*pos_y;
                *authorization=TRUE;
                message.wasp.pos_x=*pos_x;
                message.wasp.pos_y=*pos_y;
                thread_data->wasps[(message.wasp.id-1)].pos_x=*pos_x;
                thread_data->wasps[(message.wasp.id-1)].pos_y=*pos_y;
            }
            
        }
        error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
        if(error < 0){
            perror("Error sending message of connection roach");
            exit(EXIT_FAILURE);
        }
    }else{
        message.terminate=TRUE;
        error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
    }
}

void lizard_movement(ThreadData *thread_data, int *eaten_roach, remote_char_t *message, bool *authorization, direction_t *prev_orientation, void *responder, int *prev_x, int *prev_y, int *pos_x, int *pos_y){

    int error;
    lizard_t *lizard_aux, *lizard_aux2;
    char dummy;
    *authorization=FALSE;

    *pos_x = message->lizard.pos_x;
    *pos_y = message->lizard.pos_y;
    //prev positions!
    *prev_orientation = message->lizard.orientation;
    *prev_x = *pos_x;
    *prev_y = *pos_y;

    new_position(pos_x, pos_y, *message);
    
    dummy=is_there_lizard(thread_data->tabuleiro_lizards,*pos_x,*pos_y);
    //CHOCOU COM A CABEÇA DE ALGUM LIZARD?  ---> authorization deve estar a  FALSE
    if (dummy==' ' && is_there_wasp(thread_data->wasps, *pos_x,*pos_y) == -1)
    {   
        lizard_aux = search_lizard(thread_data->lizard_head, message->lizard.id);
        //COMEU ROACH?
        if ((*eaten_roach =is_there_cockroach(thread_data->cockroaches,*pos_x,*pos_y))!=-1)
        {   
            message->lizard.score = message->lizard.score + thread_data->cockroaches[*eaten_roach].score;
            lizard_aux->score = message->lizard.score; 
        }

        
        //DEPOIS ATUALIZAR O LIZARD NA ESTRUTURA DE DADOS CORRESPONDENTE E MANDAR MENSAGEM AO CLIENTE COM AS NOVAS POSIÇÕES
        lizard_aux->pos_x = *pos_x;
        lizard_aux->pos_y = *pos_y;
        lizard_aux->orientation = message->direction;

        //sends updates to the client
        message->lizard.pos_x=*pos_x;
        message->lizard.pos_y=*pos_y;
        message->lizard.orientation=message->direction;

        //updates tabuleiro Lizards
        pthread_mutex_lock(&myMutex);
        thread_data->tabuleiro_lizards[*pos_x][*pos_y] = lizard_aux->id;
        pthread_mutex_unlock(&myMutex);
        *authorization=TRUE;

    }
    else if(dummy!=' ') //in the case of two lizards coliding
    {   
        lizard_aux = search_lizard(thread_data->lizard_head,message->lizard.id);
        lizard_aux2 = search_lizard(thread_data->lizard_head,dummy);

        lizard_aux->score=lizard_aux->score-((lizard_aux->score+lizard_aux2->score)/2);
        lizard_aux2->score=lizard_aux2->score-((lizard_aux->score+lizard_aux2->score)/2);

        message->lizard.score=lizard_aux->score;
        *authorization=FALSE;
        //if score negative -> Terminate!!
        // if(lizard_aux->score<0){
        //     terminate_lizard(display, thread_data, *message, thread_data->publisher, *prev_orientation, responder);
        // }
    }else if (is_there_wasp(thread_data->wasps, *pos_x,*pos_y) != -1)
    {
        lizard_aux = search_lizard(thread_data->lizard_head, message->lizard.id);
        lizard_aux->score-=10;
    }
    
    
    error = zmq_send(responder,message,sizeof(*message),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

}

void cockroach_movement(ThreadData *thread_data, remote_char_t message, bool *authorization, int *prev_x, int *prev_y, int *pos_x, int *pos_y){

    //message.cockroach.pos_x/y have the coordinates before the movement
    int error;

    *prev_x = message.cockroach.pos_x;
    *prev_y = message.cockroach.pos_y;

    *pos_x= message.cockroach.pos_x;
    *pos_y=message.cockroach.pos_y;

    new_position(pos_x,pos_y,message);

    //COLLISION WITH LIZARD'S HEAD?
    if(is_there_lizard(thread_data->tabuleiro_lizards,*pos_x,*pos_y)== ' '){
        
        //atualizar na nossa estrutura de roaches
        thread_data->cockroaches[(message.cockroach.id-1)].pos_x=*pos_x;
        thread_data->cockroaches[(message.cockroach.id-1)].pos_y=*pos_y;
        
        //sends updates to client
        message.cockroach.pos_x=*pos_x;
        message.cockroach.pos_y=*pos_y;
        *authorization = TRUE;

    }

    error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

}

void wasp_movement(ThreadData *thread_data, remote_char_t message, bool *authorization, int *pos_x,int *pos_y, int *prev_x,int *prev_y)
{
    int error;
    char lid;

    lizard_t *lizard_aux;

    *prev_x = message.wasp.pos_x;
    *prev_y = message.wasp.pos_y;

    *pos_x= message.wasp.pos_x;
    *pos_y=message.wasp.pos_y;

    new_position(pos_x,pos_y,message);

    if (is_there_cockroach(thread_data->cockroaches,(*pos_x),(*pos_y))==-1)
    {
        if ((lid = is_there_lizard(thread_data->tabuleiro_lizards,*pos_x,*pos_y))==' ')
        {
            thread_data->wasps[(message.wasp.id-1)].pos_x=*pos_x;
            thread_data->wasps[(message.wasp.id-1)].pos_y=*pos_y;
            
            //sends updates to client
            message.wasp.pos_x=*pos_x;
            message.wasp.pos_y=*pos_y;
            *authorization = TRUE;
        }else{
            lizard_aux = search_lizard(thread_data->lizard_head, lid);
            lizard_aux->score-=10;
        }
    }
    
    error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }
}

void write_new_lizard_body(ThreadData *thread_data, display_message display, remote_char_t message, void *publisher, int pos_x, int pos_y){

    char dummy;
    int error;
    lizard_t *lizard_aux;

    display.informative=FALSE;
    display.x=pos_x;
    display.y=pos_y;
    display.symbol=message.lizard.id;
    pthread_mutex_lock(&myMutex);
    error = zmq_send(publisher,&display,sizeof(display),0);
    if(error < 0){
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }
    wmove(thread_data->my_win, pos_x, pos_y);
    waddch(thread_data->my_win,message.lizard.id| A_BOLD);
    pthread_mutex_unlock(&myMutex);

    switch (message.lizard.orientation)
    {
    case LEFT:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,pos_x,(pos_y+i))) != ' ')
            {
                lizard_aux = search_lizard(thread_data->lizard_head,dummy);
                if(lizard_aux->orientation == LEFT){
                    break;
                }
                else
                {
                    continue;
                }
                
                
            }
            else if (is_there_cockroach(thread_data->cockroaches,pos_x,(pos_y+i))!=-1)       //if there's a cockroach it doesn't clean the postition
            {
                continue;
            }
            else if((pos_y+i)>=0 && (pos_y+i)<WINDOW_SIZE)
            {   
                display.informative=FALSE;
                display.x=pos_x;
                display.y=pos_y+i;
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,pos_x,(pos_y+i));
                if (message.lizard.score>=50)
                {   
                    display.symbol='*';
                    waddch(thread_data->my_win,'*'|A_BOLD);
                }
                else{
                    display.symbol='.';
                    waddch(thread_data->my_win,'.'| A_BOLD);
                }
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
                
            }     
            
        }
        
        break;
    
    case RIGHT:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,pos_x,(pos_y-i))) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if(lizard_aux->orientation==RIGHT){
                    break;
                }
                else
                {
                    continue;
                }
                
                
            }
            else if (is_there_cockroach(thread_data->cockroaches,pos_x,(pos_y-i))!=-1)
            {
                continue;
            }
            else if((pos_y-i)>=0 && (pos_y-i)<WINDOW_SIZE)
            {   
                display.informative=FALSE;
                display.x=pos_x;
                display.y=pos_y-i;
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,pos_x,(pos_y-i));
                if (message.lizard.score>=50)
                {
                    display.symbol='*';
                    waddch(thread_data->my_win,'*'|A_BOLD);
                }
                else{
                    display.symbol='.';
                    waddch(thread_data->my_win,'.'| A_BOLD);
                }
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
                
            }     
            
        }
        
        break;
    
    case UP:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,(pos_x+i),pos_y)) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if(lizard_aux->orientation==UP){
                    break;
                }
                else
                {
                    continue;
                }
                
                
            }
            else if (is_there_cockroach(thread_data->cockroaches,(pos_x+i),pos_y)!=-1)
            {
                continue;
            }
            else if((pos_x+i)>=0 && (pos_x+i)<WINDOW_SIZE)
            {   
                display.informative=FALSE;
                display.x=pos_x+i;
                display.y=pos_y;
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,(pos_x+i),pos_y);
                if (message.lizard.score>=50)
                {
                    display.symbol='*';
                    waddch(thread_data->my_win,'*'|A_BOLD);
                }
                else{
                    display.symbol='.';
                    waddch(thread_data->my_win,'.'| A_BOLD);
                }
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
                
            }     
            
        }
        
        break;
    
    case DOWN:
        for (int i = 1; i <= 5; i++)
        {
            if ( (dummy=is_there_lizard(thread_data->tabuleiro_lizards,(pos_x-i),pos_y)) != ' ')
            {
                lizard_aux=search_lizard(thread_data->lizard_head,dummy);
                if(lizard_aux->orientation==DOWN){
                    break;
                }
                else
                {
                    continue;
                }
                
                
            }
            else if (is_there_cockroach(thread_data->cockroaches,(pos_x-i),pos_y)!=-1)
            {
                continue;
            }
            else if((pos_x-i)>=0 && (pos_x-i)<WINDOW_SIZE)
            {   
                display.informative=FALSE;
                display.x=pos_x-i;
                display.y=pos_y;
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,(pos_x-i),pos_y);
                if (message.lizard.score>=50)
                {
                    display.symbol='*';
                    waddch(thread_data->my_win,'*'|A_BOLD);
                }
                else{
                    display.symbol='.';
                    waddch(thread_data->my_win,'.'| A_BOLD);
                }
                error = zmq_send(publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);
            }     
            
        }
        
        break;
    
    default:
        break;
    }
}

void print_message_info(remote_char_t message, char letter_pending){

    printf("Message info: \n");
    printf("          - Lizard id : %c with score: %d; lizard position : %d %d; orientation: %d, direction selected: %d  and letter pending: %d\n", message.lizard.id, message.lizard.score, message.lizard.pos_x, message.lizard.pos_y, message.lizard.orientation, message.direction, letter_pending);
}

void print_message_info_roaches(remote_char_t message){

    printf("Message info: \n");
    printf("          - roach id : %d; roach position : %d %d;, score: %d\n", message.cockroach.id, message.cockroach.pos_x, message.cockroach.pos_y, message.cockroach.score);
}

void print_message_info_wasp(remote_char_t message){

    printf("Message info: \n");
    printf("          - wasp id : %d; wasp position : %d %d;\n", message.wasp.id, message.wasp.pos_x, message.wasp.pos_y);
}

void *bots_thread_function( void *arg ){

    // Cast the argument to the correct data type
    ThreadData* thread_data = (ThreadData*)arg;
    int wasp_id=0;
    int num_id_roaches = 0, how_many_terminated = 0;
    int num_id_wasps = 0, how_many_terminated_wasps = 0;

    int prev_x, prev_y, pos_x = -1, pos_y = -1;
    int roach_id = 0, error;
    char board_rid[5]="12345";
    int terminated_roaches[COCKROACH_LIMIT];
    int terminated_wasps[WASP_LIMIT];

    for(int z=0; z<COCKROACH_LIMIT; z++){
        terminated_roaches[z] = -1;
    }
    for(int z=0; z<WASP_LIMIT; z++){
        terminated_wasps[z] = -1;
    }

    bool authorization=FALSE;           

    /* information about the character */
    int *pos_array;

    pos_array = (int*) malloc(2*sizeof(int));

    remote_char_t message;
    display_message display;

    while (1)
    {

        authorization=FALSE;
        
        ssize_t num_bytes;

        // printf("Waiting for a message!!\n");
        if((num_bytes = zmq_recv(thread_data->bot_responder, &message, sizeof(message), 0)) > 0) {

            // print_message_info_wasp(message);

            // print_message_info_roaches(message);

            /*           ROACHES          */
            if (message.bot_type == TRUE)   
            {
                if(message.msg_type == TRUE && message.terminate == FALSE){   //####CHECK TERMINATE DAS ROACHES!!

                    roaches_connection(thread_data, terminated_roaches, &num_id_roaches, &roach_id, message, &authorization, &pos_x, &pos_y,&prev_x,&prev_y, &how_many_terminated);
                }
                /*                        MOVEMENT                          */
                else if(message.msg_type == FALSE && message.terminate == FALSE){
                    
                    cockroach_movement(thread_data, message, &authorization, &prev_x, &prev_y, &pos_x, &pos_y);
                }
                else{
                    //so acontece caso seja terminate!!
                    terminate_roach(thread_data, terminated_roaches, &message, &authorization, &how_many_terminated);
                    error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
                    if(error < 0){
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    }
                    
                }
            }
            /*           WASPS          */
            else{
                
                if(message.msg_type == TRUE && message.terminate == FALSE){   //####CHECK TERMINATE DAS ROACHES!!

                    wasps_connection(thread_data, terminated_wasps, &num_id_wasps, &wasp_id, message, &authorization, &pos_x, &pos_y, &prev_x, &prev_y, &how_many_terminated_wasps);
                }
                /*                        MOVEMENT                          */
                else if(message.msg_type == FALSE && message.terminate == FALSE){
                    
                    wasp_movement(thread_data, message, &authorization, &pos_x, &pos_y, &prev_x, &prev_y);
                }
                else{
                    //so acontece caso seja terminate!!
                    terminate_wasp(thread_data, terminated_wasps, &message, &authorization, &how_many_terminated_wasps);
                    error = zmq_send(thread_data->bot_responder,&message,sizeof(message),0);
                    if(error < 0){
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    }
                    
                }
            }
        }
        else{
            perror("Error reading");
            exit(EXIT_FAILURE);
        }

        /*          UPDATE THE BOARD        */
        if (authorization)
        {

            if (message.bot_type==TRUE)
            {
                int underlying=is_there_cockroach(thread_data->cockroaches,prev_x,prev_y);

                // print_roaches(thread_data->cockroaches);

                if(underlying == -1 || underlying==message.cockroach.id){      //checks if there was a cockroach beneath
                    pthread_mutex_lock(&myMutex);
                    wmove(thread_data->my_win, prev_x,prev_y);
                    waddch(thread_data->my_win,' ');
                    pthread_mutex_unlock(&myMutex);
                    display.x=prev_x;
                    display.y=prev_y;
                    display.symbol=' ';
                    display.informative = FALSE;
                    pthread_mutex_lock(&myMutex);
                    error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                    if(error < 0){
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    }
                    pthread_mutex_unlock(&myMutex);
                }
                else
                {
                    pthread_mutex_lock(&myMutex);
                    wmove(thread_data->my_win,prev_x,prev_y);
                    waddch(thread_data->my_win,board_rid[(thread_data->cockroaches[(underlying-1)].score-1)]|A_BOLD);
                    pthread_mutex_unlock(&myMutex);
                    display.x=prev_x;
                    display.y=prev_y;
                    display.symbol=board_rid[(thread_data->cockroaches[(underlying-1)].score-1)];
                    display.informative = FALSE;

                    pthread_mutex_lock(&myMutex);
                    // maybe aqui mando a mensagem para as outras threads para mostrarem no client Lizard!!
                    error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                    if(error < 0){
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    }
                    pthread_mutex_unlock(&myMutex);
                }
                
                //writing in the new one
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win, pos_x, pos_y);
                waddch(thread_data->my_win,board_rid[(message.cockroach.score-1)]| A_BOLD);
                pthread_mutex_unlock(&myMutex);
                display.x=pos_x;
                display.y=pos_y;
                display.symbol=board_rid[(message.cockroach.score-1)];
                display.informative = FALSE;
                pthread_mutex_lock(&myMutex);
                error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                pthread_mutex_unlock(&myMutex);
            }
            else if (message.bot_type==FALSE)
            {
                //clearing previous position
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,prev_x,prev_y);
                waddch(thread_data->my_win,' ');
                pthread_mutex_unlock(&myMutex);
                display.x=prev_x;
                display.y=prev_y;
                display.symbol=' ';
                display.informative=FALSE;
                pthread_mutex_lock(&myMutex);
                error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                pthread_mutex_unlock(&myMutex);

                //wrinting new position of wasp

                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,pos_x,pos_y);
                waddch(thread_data->my_win,'#');
                pthread_mutex_unlock(&myMutex);
                display.x=pos_x;
                display.y=pos_y;
                display.symbol='#';
                display.informative=FALSE;
                pthread_mutex_lock(&myMutex);
                error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                pthread_mutex_unlock(&myMutex);

            }
            
             //cockroach ####Proteger my win das roaches e wasps
            
            wrefresh(thread_data->my_win);
        }
    }

    free(pos_array);

}

void *lizards_thread_function( void *arg ){

    // Cast the argument to the correct data type
    ThreadData* thread_data = (ThreadData*)arg;

    // Access the data within the structure
    // int thread_id = thread_data->unique_id; //id of each thread!!

    void *responder = zmq_socket(context, ZMQ_REP);

    // Connect to the ZMQ_ROUTER socket
    const char *dealer_address = "inproc://back-end";

    int rc = zmq_connect(responder, dealer_address);
    assert(rc == 0);

    int prev_x, prev_y, pos_x = -1, pos_y = -1;
    int eaten_roach = -1,  error;

    bool authorization=FALSE;           

    /* information about the character */
    int *pos_array, *new_pos;

    pos_array = (int*) malloc(2*sizeof(int));
    new_pos = (int*) malloc(2*sizeof(int));

    remote_char_t message;
    display_message display;
    direction_t prev_orientation;
    
    // printf("Thread ready\n");

    while (1)
    {

        authorization=FALSE;
        
        ssize_t num_bytes;

        if((num_bytes = zmq_recv(responder, &message, sizeof(message), 0)) > 0) {

            // printf("RECEIVED MESSAGE ID %d!!\n", thread_id);

            // print_message_info(message, thread_data->letter_pending);

            /*           LIZARD          */
            if (message.entity_type == TRUE)   
            {
                /*                     CONNECTION                         */
                if (message.msg_type == TRUE && message.terminate == FALSE && thread_data->num_id_atribuidos < LIZARDS_LIMIT)
                {
                    connection_lizard(thread_data, message,&prev_x,&prev_y, pos_array, responder, &pos_x, &pos_y);
                    // mostra_lista(thread_data->lizard_head);
                   
                }
                /*                          MOVEMENT                          */
                else if(message.msg_type == FALSE && message.terminate == FALSE)
                {   
                    //problema do rasto resolvido aqui ao mandar a mensagem por address; nao estava a alterar a orientaçao!!
                    //eaten roach tem de ser shared!!
                    lizard_movement(thread_data, &eaten_roach, &message, &authorization, &prev_orientation, responder, &prev_x, &prev_y, &pos_x, &pos_y);
                    // print_message_info(message);
                }

                //terminates the connection with lizard_client
                if(message.terminate == TRUE){
                    terminate_lizard(display, thread_data, message, thread_data->publisher, prev_orientation, responder);
                }
            }
        }
        else{
            perror("Error reading");
            exit(EXIT_FAILURE);
        }

        /*          UPDATE THE BOARD        */
        if (authorization)
        {
            /* draw mark on new position */
            clear_previous_lizard_body(thread_data, display, thread_data->publisher, prev_orientation, prev_x, prev_y);
            write_new_lizard_body(thread_data, display, message, thread_data->publisher, pos_x, pos_y);
            if (eaten_roach!=-1)
            {
                random_position(thread_data->tabuleiro_lizards, new_pos);
                thread_data->cockroaches[(eaten_roach-1)].pos_x=new_pos[0];
                thread_data->cockroaches[(eaten_roach-1)].pos_y=new_pos[1];

                sleep(5);
                pthread_mutex_lock(&myMutex);
                wmove(thread_data->my_win,thread_data->cockroaches[(eaten_roach-1)].pos_x,thread_data->cockroaches[(eaten_roach-1)].pos_y);
                waddch(thread_data->my_win,thread_data->cockroaches[(eaten_roach-1)].score| A_BOLD);
                wrefresh(thread_data->my_win);
                pthread_mutex_unlock(&myMutex);

                display.informative=FALSE;
                display.x=thread_data->cockroaches[(eaten_roach-1)].pos_x;
                display.y=thread_data->cockroaches[(eaten_roach-1)].pos_y;
                display.symbol=thread_data->cockroaches[(eaten_roach-1)].score;
                pthread_mutex_lock(&myMutex);
                error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
                if(error < 0){
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                pthread_mutex_lock(&myMutex);
            }
            eaten_roach=-1;

            //Display do SCORE!
            // display.informative=TRUE;
            // show_lizard_scores(thread_data->lizard_head, &display);
            // pthread_mutex_lock(&myMutex);
            // error = zmq_send(thread_data->publisher,&display,sizeof(display),0);
            // if(error < 0){
            //     perror("Error sending message");
            //     exit(EXIT_FAILURE);
            // }
            // pthread_mutex_lock(&myMutex);
            
            wrefresh(thread_data->my_win);
        }

    }
	  
    zmq_close(responder);
    zmq_ctx_shutdown(context);
    //free memory allocated!
    // free(cockroaches);
    free(pos_array);
    // free_tabuleiro(tabuleiro_lizards);

    return 0;

}

int main (void)
{

    context = zmq_ctx_new ();

    ThreadData thread_data;
    
    pthread_t workers[NUM_THREADS];
    // int vector_int[NUM_THREADS];

    thread_data.vector_heads = (lizard_t**) malloc(2*sizeof(lizard_t*));

    for(int z=0; z<2; z++){
        thread_data.vector_heads[z] = (lizard_t*) malloc(sizeof(lizard_t));
    }

    thread_data.cockroaches = (cockroach_t*) malloc(COCKROACH_LIMIT*sizeof(cockroach_t));
    thread_data.wasps = (wasp_t*) malloc(WASP_LIMIT*sizeof(wasp_t));

    thread_data.tabuleiro_lizards = create_tabuleiro();
    init_tabuleiro(thread_data.tabuleiro_lizards);

    char lizard_id = 'a';
    init_alfabeto(thread_data.alfabeto, lizard_id);

    thread_data.lizard_head = iniLista();
    thread_data.terminated_lizard_head = iniLista();

    thread_data.num_id_atribuidos = 0;
    thread_data.letter_pending = 0;
    thread_data.sync = 0;

    // ncurses initialization
	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    thread_data.my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(thread_data.my_win, 0 , 0);
	wrefresh(thread_data.my_win);

    //  Socket facing clients
    void *frontend = zmq_socket (context, ZMQ_ROUTER);
    int rc = zmq_bind (frontend, "tcp://*:5555");
    assert (rc == 0);

    //  Socket facing workers
    void *backend_lizards = zmq_socket (context, ZMQ_DEALER);
    rc = zmq_bind (backend_lizards, "inproc://back-end");
    assert (rc == 0);

    thread_data.bot_responder = zmq_socket (context, ZMQ_REP);
    rc = zmq_bind (thread_data.bot_responder, "tcp://*:5557");
    assert (rc == 0);

    //sockets for pub sub communication
    thread_data.publisher = zmq_socket(context,ZMQ_PUB);
    assert(thread_data.publisher!=NULL);
    int pb = zmq_bind(thread_data.publisher, "tcp://*:5558");
    assert(pb==0);

    long int worker_nbr;

    for (worker_nbr = 0; worker_nbr < NUM_THREADS-1; worker_nbr++) {

        thread_data.unique_id = worker_nbr;
        pthread_create( &workers[worker_nbr], NULL, lizards_thread_function, (void *)&thread_data);
        sleep(2); //just to ensure that threads don't create all at the same exact moment!
    }

    worker_nbr++;

    pthread_create( &workers[worker_nbr], NULL, bots_thread_function, (void *)&thread_data);

    //  Start the proxy
    zmq_proxy (frontend, backend_lizards, NULL);
    // zmq_proxy (frontend, backend_bots, NULL);

    endwin();			// End curses mode	
    zmq_close(frontend);
    zmq_close(backend_lizards);
    zmq_ctx_destroy(context);

    //  We never get here...
    return 0;
}

