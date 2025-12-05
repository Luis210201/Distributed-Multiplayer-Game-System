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



typedef struct {

    wasp_t *wasps;
    int n_wasps;
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


void *key(void *arg){

    ThreadData* thread_data = (ThreadData*)arg;

    char command;
    int error,n;

    remote_char_t message;
    message.entity_type=FALSE;
    message.bot_type=FALSE;
    
    scanf("Type some letter to terminate:%c",&command);

    //send message.Terminate 
    for (int i = 0; i < thread_data->n_wasps; i++)
    {
        printf("Terminating client\n");
        message.wasp.id=thread_data->wasps[i].id;
        message.wasp.pos_x = thread_data->wasps[i].pos_x;
        message.wasp.pos_y = thread_data->wasps[i].pos_y;
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

/*
*   The roach-client can be called in two ways:
*   1 - ./roaches address port                          (random number of roaches between 1-10)
*   2 - ./roaches address port number_of_roaches        (choosen number of roaches)
*/


int main(int argc, char **argv)
{   
    int port;
    char *address; //needs to be char because of the dots
    address = (char*) malloc(13);    
    int error;

    pthread_t waiting_key;

    ThreadData thread_data;
    thread_data.n_wasps=0;


    if(argc == 3){
        strcpy(address, argv[1]);
        port = atoi(argv[2]);
        thread_data.n_wasps = rand()%9+1;
    }
    else if (argc == 4)
    {
        strcpy(address, argv[1]);
        port = atoi(argv[2]);
        thread_data.n_wasps= atoi(argv[3]);
        if (thread_data.n_wasps<1 || thread_data.n_wasps>10)
        {
            printf("The number of roaches must be between 1 and 10");
            exit(1);
        }
    }
    else{
        exit(1);
    }

    thread_data.wasps = malloc(thread_data.n_wasps*sizeof(wasp_t));

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
    
    int random_selection[thread_data.n_wasps];
    for (int i = 0; i < thread_data.n_wasps; i++)
    {
        random_selection[i]=i;
    }

    remote_char_t message;

    message.entity_type = FALSE;    // FALSE for wasps (see remote-char.h)
    
    /*sending connection message to the server*/

    message.bot_type = FALSE;

    message.msg_type=TRUE;  //true for connection (see remote-char.h)

    __ssize_t n;

    /*choosing a random order of wasps*/
    fisherYatesShuffle(random_selection, thread_data.n_wasps);
    

    /*-------------CONNECTION--------------*/

    /*sending the randomly choosen wasps*/
    for (int i = 0; i < thread_data.n_wasps; i++)
    {
        message.wasp=thread_data.wasps[random_selection[i]];

        error = zmq_send(thread_data.requester,&message,sizeof(remote_char_t),0); 
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }

        if((n = zmq_recv(thread_data.requester, &message, sizeof(message), 0)) > 0){
            
            if(message.terminate){                                          //the server doesn't wnat any more roaches
                printf("The server has enough wasps!");
                exit(1);
            }else{
                
                thread_data.wasps[random_selection[i]].id=message.wasp.id;   //receiving the id to store in our data for later communication
                thread_data.wasps[random_selection[i]].pos_x=message.wasp.pos_x;
                thread_data.wasps[random_selection[i]].pos_y=message.wasp.pos_y;
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
        fisherYatesShuffle(random_selection, thread_data.n_wasps);

        /*sending the randomly choosen wasps*/
        for (int i = 0; i < thread_data.n_wasps; i++)
        {
            message.wasp=thread_data.wasps[random_selection[i]];     //chooses the wasp randomly
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
                thread_data.wasps[random_selection[i]].pos_x=message.wasp.pos_x;
                thread_data.wasps[random_selection[i]].pos_y=message.wasp.pos_y;
            }
            
            
            sleep(rand()%5);      //wait a random amount of time before controlling the next roach

        }

        
        
    }
    


}
