//should use TCP ZMQ communication
//the adress and the port are given to the program -> command line
#include <ncurses.h>
#include "remote-char.h"
#include "display_messages.h"
#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

void display_scores(info information){
    
    int j=0;

    for(int i = 0; i < 26; i++){

        if(information.score_array[i] != -1 && j==0){
            printf("DISPLAY SCORES information.score_array[i]: %d!\n", information.score_array[i]);
            // mvprintw(31,0+j,"Lizard %c has a score of %d\n", information.symbol_array[i], information.score_array[i]);
            // refresh();
            j+=15;
        }
        else{
            // printf("VOU EMBORA\n");
            // refresh();
            break;
        }
    }

}

void print_display_info(display_message message){

    printf("Message info: \n");
    printf("          - roach id : %d; roach position : %d %d;\n", message.symbol, message.x, message.y);
}

void* game_display(void *subscriber){

    int comm_flag;
    int size;

    display_message message;

    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
    wrefresh(my_win);

    // printf("Thread ready\n");

    while (TRUE)
    {

        // printf("WAITING TO RECEIVE MESSAGE!!\n");
        if((comm_flag=zmq_recv(subscriber,&message,sizeof(display_message)-1,0)) > 0){

            // print_display_info(message);
            
            if (message.informative == FALSE)
            {
                
                wmove(my_win,message.x,message.y);
                waddch(my_win,message.symbol|A_BOLD);
                wrefresh(my_win);
            }
            else
            {   
                size = 25;
                message.information.symbol_array[size] = '\0';
                display_scores(message.information);
            }
        }
        else
        {
            perror("Error!");
            sleep(5);
            exit(1);
        }
        
        // wrefresh(my_win);
    }

}

int main(int argc, char **argv)
{   
    int port_req, port_sub;
    char *address; //needs to be char because of the dots

    address = (char*) malloc(13);
    
    int error;
    int check_connection;
    int n_bytes;
    int comm_flag;
    int size;
    pthread_t worker;

    display_message display;

    if(argc == 4){
        strcpy(address, argv[1]);
        port_req = atoi(argv[2]);
        port_sub = atoi(argv[3]);
    }
    else{
        printf("Falta informaçao!\n");
        exit(0);
    }

    remote_char_t message;

    // Create a dynamic connection string
    char connection_string1[150], connection_string2[150]; 
    char string[] = "tcp://";
    char dois_pontos = ':';
    snprintf(connection_string1, sizeof(connection_string1), "%s%s%c%d", string, address, dois_pontos, port_req);
    snprintf(connection_string2, sizeof(connection_string2), "%s%s%c%d", string, address, dois_pontos, port_sub);

    // error handling
    printf ("Connecting to the server\n");
    void *context = zmq_ctx_new ();
    assert(context != NULL);

    void *requester = zmq_socket (context, ZMQ_REQ);
    assert(requester!= NULL);

    error = zmq_connect (requester, connection_string1);
    if(error != 0){ 
        perror("Error connecting!!");
        exit(EXIT_FAILURE);
    }

    void *subscriber = zmq_socket(context, ZMQ_SUB);
    assert(subscriber!=NULL);
    zmq_connect(subscriber,connection_string2);
    zmq_setsockopt(subscriber,ZMQ_SUBSCRIBE,"",0);

    //strategy with communication overhead. Optimize it
    //equanto nao receber um caracter -> fica aqui a espera da connection
    do{

        message.msg_type = TRUE;
        message.entity_type = TRUE;
        message.terminate = FALSE;
        message.lizard.orientation = rand()%4;  //atribui uma orientação random
        
        printf ("Sending connection message\n");
        error = zmq_send (requester, &message, sizeof(message), 0);
        if(error < 0){
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
        check_connection = zmq_recv (requester, &message, sizeof(message), 0); //checks if connection was established!!
        
    }while(check_connection < 0);

	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

    pthread_create( &worker, NULL, game_display, (void *)subscriber);

    int ch=0;

    while(1){

    	ch = getch();

        if(ch == 'q' || ch == 'Q'){

            // Send a termination message to the server
            message.terminate = TRUE;

            error = zmq_send(requester, &message, sizeof(message), 0);
            if(error < 0){
                perror("Error sending message\n");
                exit(EXIT_FAILURE);
            }

            break;

        }

        switch(ch){
            case KEY_LEFT:
                // mvprintw(32,0," Left arrow is pressed and ch -> %d", ch);
                message.direction=2;
                break;
            case KEY_RIGHT:
                // mvprintw(32,0," Right arrow is pressed and ch -> %d", ch);
                message.direction=3;
                break;
            case KEY_DOWN:
                // mvprintw(32,0," Down arrow is pressed and ch -> %d", ch);
                message.direction=1;
                break;
            case KEY_UP:
                // mvprintw(32,0," Up arrow is pressed and ch -> %d", ch);
                message.direction=0;
                break;
            default:
                ch = 'x';
                break;
        }
        // refresh();			// Print it on to the real screen 

        //this if checks if the the pressed key was any of the movement keys! if not it continues searchig for a movement key pressed!
        if(ch == 'x'){
            continue;
        }
        else{
            // printf("Sending movement message\n");
            message.msg_type = FALSE;
            message.terminate = FALSE;

            error = zmq_send(requester, &message, sizeof(message), 0);
            if(error < 0){
                perror("Error sending message\n");
                exit(EXIT_FAILURE);
            }
            if ((n_bytes=zmq_recv(requester,&message,sizeof(message),0))<0)       //this automatically updates what we will be sending regarding the poositions
            {
                perror("Error receiving message!\n");
                exit(EXIT_FAILURE);
                
            }
            else if(message.terminate){
                //if thread works! when i do this i need to do pthread join!
                exit(0);
            }
            
        }


        //receives the its new score
        // n_bytes = zmq_recv (requester, &message, sizeof(message), 0);
        // if(n_bytes < 0){
        //         perror("Error sending message");
        //         exit(EXIT_FAILURE);
        // }
        
        // mvprintw("O Novo score é de %d\n", message.lizard.score);
        // wrefresh(my_win);


    }

  	endwin();			// End curses mode		  
    zmq_close(subscriber);
    zmq_close(requester);
    zmq_ctx_shutdown(context);

	return 0;
}