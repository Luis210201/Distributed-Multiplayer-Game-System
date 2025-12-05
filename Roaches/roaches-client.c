//will be pretty similar to the things done in lizard-client!
//should use TCP ZMQ communication
//the adress and the port are given to the program -> command line
#include <ncurses.h>
#include "remote-char.h"
#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

// Structure to pass data to the thread
typedef struct {

    cockroach_t *cockroaches;
    int n_roaches;
    void *requester;
} ThreadData;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void fisherYatesShuffle(int arr[], int size) {
    for (int i = size - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        swap(&arr[i], &arr[j]);
    }
}


/*
*   The roach-client can be called in two ways:
*   1 - ./roaches address port                          (random number of roaches between 1-10)
*   2 - ./roaches address port number_of_roaches        (choosen number of roaches)
*/

void *key(void *arg){

    ThreadData* thread_data = (ThreadData*)arg;

    char command;
    int error,n;

    remote_char_t message;
    message.entity_type=FALSE;
    message.bot_type=TRUE;
    
    scanf("Type some letter to terminate:%c",&command);

    //send message.Terminate 
    for (int i = 0; i < thread_data->n_roaches; i++)
    {
        printf("Terminating client\n");
        message.cockroach.id=thread_data->cockroaches[i].id;
        message.cockroach.pos_x = thread_data->cockroaches[i].pos_x;
        message.cockroach.pos_y = thread_data->cockroaches[i].pos_y;
        message.terminate=TRUE;

        error = zmq_send(thread_data->requester,&message,sizeof(remote_char_t),0); 
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
        
        if((n = zmq_recv(thread_data->requester, &message, sizeof(message), 0)) < 0){
            perror("Communication problem");
        }
        
    }

    exit(0);
    
    return 0;
}

int main(int argc, char **argv)
{   
    int port;
    char *address; //needs to be char because of the dots
    address = (char*) malloc(13);    
    int error;
    
    ThreadData thread_data;
    thread_data.n_roaches = 0; //escolher o número de roaches a criar e controlar


    pthread_t waiting_key;

    if(argc == 3){
        strcpy(address, argv[1]);
        port = atoi(argv[2]);
        thread_data.n_roaches = rand()%9+1;
    }
    else if (argc == 4)
    {
        strcpy(address, argv[1]);
        port = atoi(argv[2]);
        thread_data.n_roaches = atoi(argv[3]);
        if (thread_data.n_roaches<1 || thread_data.n_roaches>10)
        {
            printf("The number of roaches must be between 1 and 10");
            exit(1);
        }
    }
    else{
        exit(1);
    }

    /*connecting to the server*/
    char connection_string[150]; 
    char string[] = "tcp://";
    char dois_pontos = ':';
    snprintf(connection_string, sizeof(connection_string), "%s%s%c%d", string, address, dois_pontos, port);

    printf ("Connecting to the server\n");
    void *context = zmq_ctx_new ();
    assert(context != NULL);
    thread_data.requester = zmq_socket (context, ZMQ_REQ);
    assert(thread_data.requester!= NULL);
    error = zmq_connect (thread_data.requester, connection_string);
    if(error != 0){ 
        perror("Error connecting!");
        exit(EXIT_FAILURE);
    }


    thread_data.cockroaches = malloc(thread_data.n_roaches*sizeof(cockroach_t));
    
    int random_selection[thread_data.n_roaches];
    for (int i = 0; i < thread_data.n_roaches; i++)
    {
        random_selection[i]=i;
    }
    printf("N roaches: %d\n", thread_data.n_roaches);

    /*Initializing the cockroaches with a score (which will be fixed for each instance of roaches-client) and direction (that will change during the game)*/
    for (int i = 0; i < thread_data.n_roaches; i++)
    {
        //score
        thread_data.cockroaches[i].score = rand()%4 + 1;
    }


    remote_char_t message;

    message.entity_type = FALSE;    // FALSE for cockroaches (see remote-char.h)
    
    /*sending connection message to the server*/
    message.bot_type = TRUE;

    message.msg_type=TRUE;  //true for connection (see remote-char.h)

    __ssize_t n;

    /*choosing a random order of cockroaches*/
    fisherYatesShuffle(random_selection, thread_data.n_roaches);
    

    /*-------------CONNECTION--------------*/

    /*sending the randomly choosen cockroaches*/
    for (int i = 0; i < thread_data.n_roaches; i++)
    {
        message.cockroach=thread_data.cockroaches[random_selection[i]];

        error = zmq_send(thread_data.requester,&message,sizeof(remote_char_t),0); 
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }

        if((n = zmq_recv(thread_data.requester, &message, sizeof(message), 0)) > 0){
            
            if(message.terminate){                                          //the server doesn't wnat any more roaches
                printf("The server has enough cockroaches!");
                exit(1);
            }else{
                
                thread_data.cockroaches[random_selection[i]].id=message.cockroach.id;   //receiving the id to store in our data for later communication
                thread_data.cockroaches[random_selection[i]].pos_x=message.cockroach.pos_x;
                thread_data.cockroaches[random_selection[i]].pos_y=message.cockroach.pos_y;
            }
            
        }

        sleep(rand()%3+1);
    }

    /*-----------------MOVEMENT-----------------*/

    message.msg_type=FALSE; //see remote-char.h
    message.terminate=  FALSE; //just making sure 

    pthread_create(&waiting_key,NULL,key,(void *) &thread_data);

    while (1)
    {
        /*choosing a random order of roaches to move*/
        fisherYatesShuffle(random_selection, thread_data.n_roaches);

        /*sending the randomly choosen cockroaches*/
        for (int i = 0; i < thread_data.n_roaches; i++)
        {
            message.cockroach=thread_data.cockroaches[random_selection[i]];     //chooses the cockroach randomly
            message.direction=rand()%4;                             //assigns a certain direction where to move

            error = zmq_send(thread_data.requester,&message,sizeof(remote_char_t),0); 
            if(error < 0){
                perror("Error sending message");
                exit(EXIT_FAILURE);
            }
            
            if((n = zmq_recv(thread_data.requester, &message, sizeof(message), 0)) < 0){
                perror("Communication problem");
            }
            else
            {
                thread_data.cockroaches[random_selection[i]].pos_x=message.cockroach.pos_x;
                thread_data.cockroaches[random_selection[i]].pos_y=message.cockroach.pos_y;
            }
            
            
            sleep(rand()%5);      //wait a random amount of time before controlling the next roach

        }
        
    }
}
